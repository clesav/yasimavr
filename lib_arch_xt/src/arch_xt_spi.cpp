/*
 * arch_xt_spi.cpp
 *
 *  Copyright 2021-2025 Clement Savergne <csavergne@yahoo.com>

    This file is part of yasim-avr.

    yasim-avr is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    yasim-avr is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.
 */

//=======================================================================================

#include "arch_xt_spi.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "ioctrl_common/sim_spi.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE

using namespace SPI;


//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base + offsetof(SPI_t, reg))

#define REG_OFS(reg) \
    offsetof(SPI_t, reg)


static const unsigned long ClockFactors[] = { 4, 16, 64, 128 };
static const SerialMode SERIAL_MODES[] = { Mode0, Mode1, Mode2, Mode3 };

enum ControllerMode { Mode_Disabled, Mode_Host, Mode_Client };


//=======================================================================================

class ArchXT_SPI::_PinDriver : public PinDriver {

public:

    _PinDriver(_Controller& ctrl, ctl_id_t id);

    void set_mode(ControllerMode mode);
    void set_line_state(SPI::Line line, bool state);

    virtual Pin::controls_t override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls) override;
    virtual void digital_state_changed(pin_index_t pin_index, bool state) override;

private:

    _Controller& m_ctrl;
    bool m_is_host;
    bool m_line_states[3];

};


class ArchXT_SPI::_Controller : public EndPoint, public CycleTimer {


public:

    explicit _Controller(ArchXT_SPI& peripheral);

    inline PinDriver& pin_driver() { return m_pin_driver; }

    void init(CycleManager& cycle_manager, Logger& logger);
    void reset();

    void set_bit_delay(cycle_count_t delay);

    void set_mode(ControllerMode mode);
    inline ControllerMode mode() const { return m_mode; }

    void set_serial_mode(SerialMode mode);

    void push_tx(uint8_t data, bool force_buffer);
    inline bool tx_pending() const { return m_tx_buf_loaded; }

    uint8_t pop_rx();
    uint8_t peek_rx() const;
    inline unsigned int rx_available() const { return m_rx_buffer.size(); }

    uint8_t mock_transfer(uint8_t rx_frame);

    using EndPoint::active;

    inline void input_clock(bool state) { set_shift_clock(state); }

    void set_selected(bool selected);
    inline bool selected() const { return m_selected; }

    virtual cycle_count_t next(cycle_count_t when) override;

protected:

    virtual void frame_completed() override;
    virtual void write_data_output(bool level) override;
    virtual bool read_data_input() override;

private:

    ArchXT_SPI& m_peripheral;
    _PinDriver m_pin_driver;
    CycleManager* m_cycle_manager;
    Logger* m_logger;
    cycle_count_t m_halfbitdelay;
    ControllerMode m_mode;
    bool m_selected;
    uint8_t m_tx_buffer;
    bool m_tx_buf_loaded;
    std::deque<uint8_t> m_rx_buffer;

    void output_clock(bool state);

};


//=======================================================================================

ArchXT_SPI::_PinDriver::_PinDriver(ArchXT_SPI::_Controller& ctrl, ctl_id_t id)
:PinDriver(id, 4)
,m_ctrl(ctrl)
,m_is_host(false)
,m_line_states{false, false, false}
{}


void ArchXT_SPI::_PinDriver::set_mode(ControllerMode mode)
{
    m_is_host = (mode == Mode_Host);

    bool en = (mode != Mode_Disabled);
    if (en != enabled(0))
        set_enabled(en);
    else
        update_pin_states();
}


void ArchXT_SPI::_PinDriver::set_line_state(Line line, bool state)
{
    m_line_states[line] = state;
    update_pin_state(line);
}


Pin::controls_t ArchXT_SPI::_PinDriver::override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls)
{
    Pin::controls_t c = gpio_controls;
    switch ((Line) pin_index) {
        case Clock: {
            c.drive = (unsigned char) m_line_states[Clock];
            if (!m_is_host)
                c.dir = 0;
        } break;

        case MISO: {
            c.drive = (unsigned char) m_line_states[MISO];
            if (m_is_host || !m_ctrl.selected())
                c.dir = 0;
        } break;

        case MOSI: {
            c.drive = (unsigned char) m_line_states[MOSI];
            if (!m_is_host)
                c.dir = 0;
        } break;

        case Select: {
            if (!m_is_host)
                c.dir = 0;
        } break;
    }
    return c;
}


void ArchXT_SPI::_PinDriver::digital_state_changed(pin_index_t pin_index, bool digstate)
{
    switch ((Line) pin_index) {
        case Clock: {
            if (!m_is_host)
                m_ctrl.input_clock(digstate);
        } break;

        case Select: {
            if (!m_is_host) {
                m_ctrl.set_selected(!digstate);
                update_pin_state(MISO);
            }
            else if (!gpio_controls(Select).dir) {
                m_ctrl.set_selected(!digstate);
            }
        } break;

        default: break;
    }
}


//=======================================================================================

ArchXT_SPI::_Controller::_Controller(ArchXT_SPI& peripheral)
:m_peripheral(peripheral)
,m_pin_driver(*this, peripheral.id())
,m_cycle_manager(nullptr)
,m_logger(nullptr)
,m_halfbitdelay(1)
,m_mode(Mode_Disabled)
,m_selected(false)
,m_tx_buffer(0x00)
,m_tx_buf_loaded(false)
{}


void ArchXT_SPI::_Controller::init(CycleManager& cycle_manager, Logger& logger)
{
    m_cycle_manager = &cycle_manager;
    m_logger = &logger;
}


void ArchXT_SPI::_Controller::reset()
{
    if (scheduled())
        m_cycle_manager->cancel(*this);

    set_mode(Mode_Disabled);
    set_active(false);
    set_shift_data(0x00);

    m_tx_buf_loaded = false;
    m_rx_buffer.clear();

    output_clock(false);

    m_pin_driver.set_line_state(MOSI, false);
    m_pin_driver.set_line_state(MISO, false);
}


void ArchXT_SPI::_Controller::set_bit_delay(cycle_count_t delay)
{
    m_halfbitdelay = delay / 2;
    if (m_halfbitdelay < 1) m_halfbitdelay = 1;
}


void ArchXT_SPI::_Controller::set_mode(ControllerMode m)
{
    if (m == m_mode) return;
    m_mode = m;

    if (m_mode == Mode_Host)
        output_clock(serial_mode() >= SPI::Mode2);

    set_selected(m_selected);
    m_pin_driver.set_mode(m);
}


void ArchXT_SPI::_Controller::set_serial_mode(SPI::SerialMode mode)
{
    EndPoint::set_serial_mode(mode);

    if (m_mode == Mode_Host)
        output_clock(mode >= Mode2);
}


void ArchXT_SPI::_Controller::push_tx(uint8_t data, bool force_buffer)
{
    if (active() || force_buffer) {
        m_tx_buffer = data;
        m_tx_buf_loaded = true;
    } else {
        set_shift_data(data);
    }

    //In host mode, if the interface is inactive, the transfer can start
    if (m_mode == Mode_Host && !active()) {
        set_active(true);
        m_cycle_manager->delay(*this, m_halfbitdelay);

        //If an RX frame is present in the shift register, it's lost.
        if (m_rx_buffer.size() == 3)
            m_rx_buffer.pop_back();

        m_logger->dbg("Host frame transfer started.");
    }
}


uint8_t  ArchXT_SPI::_Controller::pop_rx()
{
    if (!m_rx_buffer.size()) return 0x00;
    uint8_t v = m_rx_buffer.front();
    m_rx_buffer.pop_front();
    return v;
}


uint8_t  ArchXT_SPI::_Controller::peek_rx() const
{
    return m_rx_buffer.size() ? m_rx_buffer.front() : 0x00;
}


cycle_count_t ArchXT_SPI::_Controller::next(cycle_count_t when)
{
    //Toggle the clock line
    output_clock(!shift_clock());

    //If the frame is not complete, continue with it
    if (!complete_frame())
        return when + m_halfbitdelay;

    //Signal the success of the transfer
    m_logger->dbg("Host frame transfer ended.");
    //Is there another frame to send ? if so, restart a transfer and reschedule
    //the timer
    if (m_tx_buf_loaded) {
        set_shift_data(m_tx_buffer);
        m_tx_buf_loaded = false;
        return when + m_halfbitdelay;
    } else {
        //If not, deactivate the interface, return the clock line to idle state and stop the timer
        set_active(false);
        output_clock(serial_mode() >= Mode2);
        return 0;
    }
}


void ArchXT_SPI::_Controller::frame_completed()
{
    //The receive buffer has a normal limit of two but the 1st one represents the shift register that is only
    //discarded on the start of the next transfer.
    if (m_rx_buffer.size() < 3)
        m_rx_buffer.push_back(shift_data());

    m_peripheral.frame_completed();
}


void ArchXT_SPI::_Controller::write_data_output(bool level)
{
    if (m_mode == Mode_Host)
        m_pin_driver.set_line_state(MOSI, level);
    else if (m_mode == Mode_Client)
        m_pin_driver.set_line_state(MISO, level);
}


bool ArchXT_SPI::_Controller::read_data_input()
{
    switch (m_mode) {
        case Mode_Host:
            return m_pin_driver.pin_state(MISO).digital_value();
        case Mode_Client:
            return m_pin_driver.pin_state(MOSI).digital_value();
        default:
            return false;
    }
}


void ArchXT_SPI::_Controller::output_clock(bool level)
{
    set_shift_clock(level);
    m_pin_driver.set_line_state(Clock, level);
}


void ArchXT_SPI::_Controller::set_selected(bool selected)
{
    m_selected = selected;

    if (m_mode == Mode_Host) {
        if (selected)
            m_peripheral.host_selected();
    }
    else if (m_mode == Mode_Client) {
        if (selected && !active()) {
            set_active(true);
            m_logger->dbg("Client frame transfer started.");
        }
        else if (!selected && active()) {
            set_active(false);
            if (complete_frame())
                m_logger->dbg("Client frame transfer ended ok.");
            else
                m_logger->dbg("Client frame transfer ended nok.");
        }
    }
}


uint8_t ArchXT_SPI::_Controller::mock_transfer(uint8_t rx_frame)
{
    uint8_t tx_frame = shift_data();
    set_shift_data(rx_frame);
    if (m_rx_buffer.size() == 3)
        m_rx_buffer.pop_back();
    m_rx_buffer.push_back(rx_frame);
    m_peripheral.frame_completed();
    return tx_frame;
}


//=======================================================================================

#define NORM_ENABLE_MASK (SPI_IE_bm)
#define BUF_ENABLE_MASK  (SPI_RXCIE_bm | SPI_TXCIE_bm | SPI_DREIE_bm | SPI_SSIE_bm)
#define NORM_FLAG_MASK   (SPI_IF_bm | SPI_WRCOL_bm)
#define BUF_FLAG_MASK    (SPI_RXCIF_bm | SPI_TXCIF_bm | SPI_DREIF_bm | SPI_SSIF_bm | SPI_BUFOVF_bm)
#define NORM_INTR_MASK   (SPI_IF_bm)
#define BUF_INTR_MASK    (SPI_RXCIF_bm | SPI_TXCIF_bm | SPI_DREIF_bm | SPI_SSIF_bm)

class ArchXT_SPI::_InterruptHandler : public InterruptHandler {

public:

    _InterruptHandler();

    bool init(Device& device, const reg_addr_t& reg_enable, const reg_addr_t& reg_flag, int_vect_t vector);
    void reset();

    void set_buffer_enabled(bool enabled);
    void set_flag(uint8_t mask);
    void clear_flag(uint8_t mask);
    void clear_flag_from_ioreg(uint8_t mask);
    void update_from_ioreg();
    void update_from_data_access();

private:

    int_vect_t m_vector;
    bool m_buf_enabled;
    IO_Register* m_reg_enable;
    IO_Register* m_reg_flag;
    uint8_t m_reg_cleared_flags;

    bool flag_raised() const;

};


ArchXT_SPI::_InterruptHandler::_InterruptHandler()
:InterruptHandler()
,m_vector(AVR_INTERRUPT_NONE)
,m_buf_enabled(false)
,m_reg_enable(nullptr)
,m_reg_flag(nullptr)
,m_reg_cleared_flags(0)
{}


bool ArchXT_SPI::_InterruptHandler::init(Device& device, const reg_addr_t& reg_enable, const reg_addr_t& reg_flag, int_vect_t vector)
{
    m_reg_enable = device.core().get_ioreg(reg_enable);
    m_reg_flag = device.core().get_ioreg(reg_flag);

    m_vector = vector;
    if (m_vector > 0) {
        ctlreq_data_t d = { this, m_vector };
        return device.ctlreq(AVR_IOCTL_INTR, AVR_CTLREQ_INTR_REGISTER, &d);
    } else {
        return m_vector < 0;
    }
}


void ArchXT_SPI::_InterruptHandler::reset()
{
    m_reg_cleared_flags = 0x00;
}


void ArchXT_SPI::_InterruptHandler::set_buffer_enabled(bool enabled)
{
    m_buf_enabled = enabled;

    uint8_t enable_mask = m_buf_enabled ? BUF_ENABLE_MASK : NORM_ENABLE_MASK;
    m_reg_enable->set(m_reg_enable->value() & enable_mask);

    uint8_t flag_mask = m_buf_enabled ? BUF_FLAG_MASK : NORM_FLAG_MASK;
    m_reg_flag->set(m_reg_flag->value() & flag_mask);

    update_from_ioreg();
}


void ArchXT_SPI::_InterruptHandler::set_flag(uint8_t mask)
{
    uint8_t flag_mask = m_buf_enabled ? BUF_FLAG_MASK : NORM_FLAG_MASK;
    mask &= flag_mask;
    m_reg_flag->set(m_reg_flag->value() | mask);

    if (flag_raised())
        raise_interrupt(m_vector);
}

/**
   Clear the interrupt flag bits by AND'ing them with the mask argument.
   \return true if the interrupt is canceled as a result of the flag bit changes,
   false if the interrupt is unchanged.
*/
void ArchXT_SPI::_InterruptHandler::clear_flag(uint8_t mask)
{
    uint8_t flag_mask = m_buf_enabled ? BUF_FLAG_MASK : NORM_FLAG_MASK;
    mask &= flag_mask;
    m_reg_flag->set(m_reg_flag->value() & ~mask);

    if (!flag_raised())
        cancel_interrupt(m_vector);
}


void ArchXT_SPI::_InterruptHandler::clear_flag_from_ioreg(uint8_t mask)
{
    uint8_t flag_mask = m_buf_enabled ? BUF_FLAG_MASK : NORM_FLAG_MASK;
    m_reg_cleared_flags |= (mask & flag_mask);
}


bool ArchXT_SPI::_InterruptHandler::flag_raised() const
{
    uint8_t e, f;
    if (m_buf_enabled) {
        //In buffer mode, enable bits and flags are aligned so we can use a bitwise AND
        e = m_reg_enable->value() & BUF_ENABLE_MASK;
        f = m_reg_flag->value() & BUF_INTR_MASK;
        return e & f;
    } else {
        //In normal mode, enable bits and flags are not aligned but there's only one enable bit
        //so we can use a logical AND
        e = m_reg_enable->value() & NORM_ENABLE_MASK;
        f = m_reg_flag->value() & NORM_INTR_MASK;
        return e && f;
    }
}


void ArchXT_SPI::_InterruptHandler::update_from_ioreg()
{
    if (interrupt_raised(m_vector)) {
        if (!flag_raised())
            cancel_interrupt(m_vector);
    } else {
        if (flag_raised())
            raise_interrupt(m_vector);
    }
}


void ArchXT_SPI::_InterruptHandler::update_from_data_access()
{
    clear_flag(m_reg_cleared_flags);
    m_reg_cleared_flags = 0x00;
}


//=======================================================================================

ArchXT_SPI::ArchXT_SPI(int num, const ArchXT_SPIConfig& config)
:Peripheral(AVR_IOCTL_SPI(0x30 + num))
,m_config(config)
{
    m_ctrl = new _Controller(*this);
    m_intflag = new _InterruptHandler();
}


ArchXT_SPI::~ArchXT_SPI()
{
    delete m_ctrl;
}


bool ArchXT_SPI::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), SPI_DORD_bm | SPI_MASTER_bm | SPI_CLK2X_bm |
                               SPI_PRESC_gm | SPI_ENABLE_bm);
    add_ioreg(REG_ADDR(CTRLB), SPI_BUFEN_bm | SPI_BUFWR_bm | SPI_SSD_bm | SPI_MODE_gm);
    add_ioreg(REG_ADDR(INTCTRL), SPI_IE_bm | SPI_RXCIE_bm | SPI_TXCIE_bm | SPI_DREIE_bm |
                                 SPI_SSIE_bm );
    add_ioreg(REG_ADDR(INTFLAGS), SPI_IF_bm | SPI_WRCOL_bm | SPI_RXCIF_bm | SPI_TXCIF_bm |
                                  SPI_DREIF_bm | SPI_SSIF_bm | SPI_BUFOVF_bm);
    add_ioreg(REG_ADDR(DATA));

    status &= m_intflag->init(device, REG_ADDR(INTCTRL), REG_ADDR(INTFLAGS), m_config.iv_spi);

    m_ctrl->init(*device.cycle_manager(), logger());

    device.pin_manager().register_driver(m_ctrl->pin_driver());

    return status;
}


void ArchXT_SPI::reset()
{
    Peripheral::reset();
    m_ctrl->reset();

    SET_IOREG(INTFLAGS, SPI_DREIF);
    m_intflag->reset();
    m_intflag->update_from_ioreg();
}


bool ArchXT_SPI::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_SPI_TRANSFER) {
        uint8_t rx_frame = data->data.as_uint();
        uint8_t tx_frame = m_ctrl->mock_transfer(rx_frame);
        data->data = tx_frame;
        return true;
    }

    return false;
}


uint8_t ArchXT_SPI::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(DATA)) {
        if (m_ctrl->rx_available())
            value = m_ctrl->pop_rx();

        if (TEST_IOREG(CTRLB, SPI_BUFEN)) {
            m_intflag->clear_flag(SPI_BUFOVF_bm);
            if (!m_ctrl->rx_available())
                m_intflag->clear_flag(SPI_RXCIF_bm);
        }

        m_intflag->update_from_data_access();
    }

    return value;
}


uint8_t ArchXT_SPI::ioreg_peek_handler(reg_addr_t addr, uint8_t value)
{
    if (addr == REG_ADDR(DATA))
        value = m_ctrl->peek_rx();

    return value;
}


void ArchXT_SPI::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        //Set the mode depending on the bits ENABLE and MASTER
        if (!(data.value & SPI_ENABLE_bm))
            m_ctrl->set_mode(Mode_Disabled);
        else if (data.value & SPI_MASTER_bm)
            m_ctrl->set_mode(Mode_Host);
        else
            m_ctrl->set_mode(Mode_Client);

        //Set the bitrate depending on the fields PRESC and CLK2X
        uint8_t clk_setting = EXTRACT_F(data.value, SPI_PRESC);
        unsigned long clk_factor = ClockFactors[clk_setting];
        if (data.value & SPI_CLK2X_bm)
            clk_factor >>= 1;
        m_ctrl->set_bit_delay(clk_factor);

        //Set the data order
        m_ctrl->set_bit_order((data.value & SPI_DORD_bm) ? LSBFirst : MSBFirst);
    }

    else if (reg_ofs == REG_OFS(CTRLB)) {
        uint8_t serial_mode_ix = EXTRACT_F(data.value, SPI_MODE);
        m_ctrl->set_serial_mode(SERIAL_MODES[serial_mode_ix]);

        m_intflag->set_buffer_enabled(data.value & SPI_BUFEN_bm);
    }

    //Writing to DATA emits the value, if TX is enabled
    else if (reg_ofs == REG_OFS(INTCTRL)) {
        m_intflag->update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(INTFLAGS)) {
        write_ioreg(REG_ADDR(INTFLAGS), data.old);

        if (TEST_IOREG(CTRLB, SPI_BUFEN)) {
            m_intflag->clear_flag(data.value & (SPI_RXCIF_bm | SPI_TXCIF_bm | SPI_SSIF_bm | SPI_BUFOVF_bm));
        }

        m_intflag->clear_flag_from_ioreg(data.value);
    }

    else if (reg_ofs == REG_OFS(DATA)) {
        if (!TEST_IOREG(CTRLA, SPI_ENABLE)) {
            logger().dbg("Writing data but the peripheral is disabled.");
        }
        else if (TEST_IOREG(CTRLB, SPI_BUFEN)) {
            if (m_ctrl->rx_available() == 3) {
                logger().dbg("Transfer start with buffer overflow condition, the data in shift is lost.");
                m_intflag->set_flag(SPI_BUFOVF_bm);
            }

            logger().dbg("Pushing TX data.");
            bool force_buffer = m_ctrl->mode() == Mode_Client && !TEST_IOREG(CTRLB, SPI_BUFWR);
            m_ctrl->push_tx(data.value, force_buffer);

            if (m_ctrl->tx_pending())
                m_intflag->clear_flag(SPI_DREIF_bm);
        } else {
            if (m_ctrl->active())
                m_intflag->set_flag(SPI_WRCOL_bm);
            else
                m_ctrl->push_tx(data.value, false);
        }
    }
}


void ArchXT_SPI::frame_completed()
{
    logger().dbg("Frame transfer completed.");

    //If Buffer Mode is enabled
    if (TEST_IOREG(CTRLB, SPI_BUFEN)) {
        //Set the Receive Complete and Data Register Empty Interrupt Flags
        m_intflag->set_flag(SPI_RXCIF_bm | SPI_DREIF_bm);

        //If there is no further data to transmit, set the TXC Interrupt Flag
        if (!m_ctrl->tx_pending())
            m_intflag->set_flag(SPI_TXCIF_bm);
    } else {
        m_intflag->set_flag(SPI_IF_bm);
        //In normal mode, the rx deque can only keep one frame
        while (m_ctrl->rx_available() > 1)
            m_ctrl->pop_rx();
    }
}


void ArchXT_SPI::host_selected()
{
    if (!TEST_IOREG(CTRLB, SPI_SSD)) {
        logger().dbg("Host selected, switching to client mode.");
        CLEAR_IOREG(CTRLA, SPI_MASTER);
        m_ctrl->set_mode(Mode_Client);
        if (TEST_IOREG(CTRLB, SPI_BUFEN))
            m_intflag->set_flag(SPI_SSIF_bm);
        else
            m_intflag->set_flag(SPI_IF_bm);
    }
}
