/*
 * arch_avr_spi.h
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

#include "arch_avr_spi.h"
#include "ioctrl_common/sim_spi.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE

using namespace SPI;


//=======================================================================================

#define HOOKTAG_SPI         0
#define HOOKTAG_PIN         1

static const uint32_t ClockFactors[] = {4, 16, 64, 128};

enum ControllerMode {
    Mode_Disabled,
    Mode_Host,
    Mode_Client
};


//=======================================================================================

class ArchAVR_SPI::_PinDriver : public PinDriver {

public:

    _PinDriver(_Controller& ctrl, ctl_id_t id);

    void set_mode(ControllerMode mode);
    void set_line_state(Line line, bool state);

    virtual Pin::controls_t override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls) override;
    virtual void digital_state_changed(pin_index_t pin_index, bool state) override;

private:

    _Controller& m_ctrl;
    bool m_enabled;
    bool m_is_host;
    bool m_line_states[3];

};


class ArchAVR_SPI::_Controller : public EndPoint, public CycleTimer {


public:

    explicit _Controller(ArchAVR_SPI& peripheral);
    virtual ~_Controller() {}

    inline PinDriver& pin_driver() { return m_pin_driver; }

    void init(CycleManager& cycle_manager, Logger& logger);
    void reset();

    void set_bit_delay(cycle_count_t delay);

    void set_mode(ControllerMode mode);
    inline ControllerMode mode() const { return m_mode; }

    void set_serial_mode(SerialMode mode);

    void push_tx(uint8_t data);

    uint8_t pop_rx();
    uint8_t peek_rx() const;
    inline bool rx_available() const { return m_rx_buf_loaded; }

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

    ArchAVR_SPI& m_peripheral;
    _PinDriver m_pin_driver;
    CycleManager* m_cycle_manager;
    Logger* m_logger;
    cycle_count_t m_halfbitdelay;
    ControllerMode m_mode;
    bool m_selected;
    uint8_t m_rx_buffer;
    bool m_rx_buf_loaded;

    void output_clock(bool state);

};


//=======================================================================================

ArchAVR_SPI::_PinDriver::_PinDriver(_Controller& ctrl, ctl_id_t id)
:PinDriver(id, 4)
,m_ctrl(ctrl)
,m_enabled(false)
,m_is_host(false)
,m_line_states{false, false, false}
{}


void ArchAVR_SPI::_PinDriver::set_mode(ControllerMode mode)
{
    m_is_host = (mode == Mode_Host);

    bool en = (mode != Mode_Disabled);
    if (en != m_enabled) {
        m_enabled = en;
        for (pin_index_t i = 0; i < 4; ++i)
            set_enabled(i, en);
    } else {
        for (pin_index_t i = 0; i < 4; ++i)
            update_pin_state(i);
    }
}


void ArchAVR_SPI::_PinDriver::set_line_state(SPI::Line line, bool state)
{
    m_line_states[line] = state;
    update_pin_state(line);
}


Pin::controls_t ArchAVR_SPI::_PinDriver::override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls)
{
    Pin::controls_t c = gpio_controls;
    switch ((Line) pin_index) {
        case Clock: {
            c.drive = m_line_states[Clock];
            if (!m_is_host)
                c.dir = 0;
        } break;

        case MISO: {
            c.drive = m_line_states[MISO];
            if (m_is_host || !m_ctrl.selected())
                c.dir = 0;
        } break;

        case MOSI: {
            c.drive = m_line_states[MOSI];
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


void ArchAVR_SPI::_PinDriver::digital_state_changed(pin_index_t pin_index, bool digstate)
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

ArchAVR_SPI::_Controller::_Controller(ArchAVR_SPI& peripheral)
:m_peripheral(peripheral)
,m_pin_driver(*this, peripheral.id())
,m_cycle_manager(nullptr)
,m_logger(nullptr)
,m_halfbitdelay(1)
,m_mode(Mode_Disabled)
,m_selected(false)
,m_rx_buffer(0x00)
,m_rx_buf_loaded(false)
{}


void ArchAVR_SPI::_Controller::init(CycleManager& cycle_manager, Logger& logger)
{
    m_cycle_manager = &cycle_manager;
    m_logger = &logger;
}


void ArchAVR_SPI::_Controller::reset()
{
    if (scheduled())
        m_cycle_manager->cancel(*this);

    set_mode(Mode_Disabled);
    set_active(false);
    set_shift_data(0x00);

    m_rx_buf_loaded = false;

    output_clock(false);

    m_pin_driver.set_line_state(SPI::MOSI, false);
    m_pin_driver.set_line_state(SPI::MISO, false);
}


void ArchAVR_SPI::_Controller::set_bit_delay(cycle_count_t delay)
{
    m_halfbitdelay = delay / 2;
    if (m_halfbitdelay < 1) m_halfbitdelay = 1;
}


void ArchAVR_SPI::_Controller::set_mode(ControllerMode m)
{
    if (m == m_mode) return;
    m_mode = m;

    if (m_mode == Mode_Host)
        output_clock(serial_mode() >= Mode2);
    else if (scheduled())
        m_cycle_manager->cancel(*this);

    set_active(false);

    set_selected(m_selected);
    m_pin_driver.set_mode(m);
}


void ArchAVR_SPI::_Controller::set_serial_mode(SerialMode mode)
{
    EndPoint::set_serial_mode(mode);

    if (m_mode == Mode_Host)
        output_clock(mode >= Mode2);
}


void ArchAVR_SPI::_Controller::push_tx(uint8_t data)
{
    set_shift_data(data);

    //If this is the first transfer, we need to start the timer
    if (m_mode == Mode_Host && !active()) {
        set_active(true);
        m_cycle_manager->delay(*this, m_halfbitdelay);
        m_logger->dbg("Host frame transfer started.");
    }
}


uint8_t  ArchAVR_SPI::_Controller::pop_rx()
{
    m_rx_buf_loaded = false;
    return m_rx_buffer;
}


uint8_t  ArchAVR_SPI::_Controller::peek_rx() const
{
    return m_rx_buffer;
}


cycle_count_t ArchAVR_SPI::_Controller::next(cycle_count_t when)
{
    //Toggle the clock line
    output_clock(!shift_clock());

    //If the frame is not complete, continue with it
    if (!complete_frame())
        return when + m_halfbitdelay;

    //Signal the success of the transfer
    m_logger->dbg("Host frame transfer ended.");
    set_active(false);
    output_clock(serial_mode() >= Mode2);
    return 0;
}


void ArchAVR_SPI::_Controller::frame_completed()
{
    m_rx_buffer = shift_data();
    m_rx_buf_loaded = true;
    m_peripheral.frame_completed();
}


void ArchAVR_SPI::_Controller::write_data_output(bool level)
{
    if (m_mode == Mode_Host)
        m_pin_driver.set_line_state(MOSI, level);
    else if (m_mode == Mode_Client)
        m_pin_driver.set_line_state(MISO, level);
}


bool ArchAVR_SPI::_Controller::read_data_input()
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


void ArchAVR_SPI::_Controller::output_clock(bool level)
{
    set_shift_clock(level);
    m_pin_driver.set_line_state(Clock, level);
}


void ArchAVR_SPI::_Controller::set_selected(bool selected)
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


uint8_t ArchAVR_SPI::_Controller::mock_transfer(uint8_t rx_frame)
{
    uint8_t tx_frame = shift_data();
    set_shift_data(rx_frame);
    m_rx_buffer = rx_frame;
    m_rx_buf_loaded = true;
    m_peripheral.frame_completed();
    return tx_frame;
}


//=======================================================================================

ArchAVR_SPI::ArchAVR_SPI(uint8_t num, const ArchAVR_SPIConfig& config)
:Peripheral(AVR_IOCTL_SPI(0x30 + num))
,m_config(config)
,m_intflag(true)
,m_intflag_accessed(false)
{
    m_ctrl = new _Controller(*this);
}


ArchAVR_SPI::~ArchAVR_SPI()
{
    delete m_ctrl;
}


bool ArchAVR_SPI::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.reg_data);
    add_ioreg(m_config.rb_enable);
    add_ioreg(m_config.rb_int_enable);
    add_ioreg(m_config.rb_int_flag, true);
    add_ioreg(m_config.rb_mode);
    add_ioreg(m_config.rb_cpol);
    add_ioreg(m_config.rb_cpha);
    add_ioreg(m_config.rb_dord);
    add_ioreg(m_config.rb_clock);
    add_ioreg(m_config.rb_clock2x);
    add_ioreg(m_config.rb_wcol, true);

    status &= m_intflag.init(device,
                             m_config.rb_int_enable,
                             m_config.rb_int_flag,
                             m_config.iv_spi);

    m_ctrl->init(*device.cycle_manager(), device.logger());

    status &= device.pin_manager().register_driver(m_ctrl->pin_driver());

    return status;
}


void ArchAVR_SPI::reset()
{
    m_ctrl->reset();
    update_framerate();
    update_serial_config();
    m_intflag_accessed = false;
}


bool ArchAVR_SPI::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_SPI_TRANSFER) {
        uint8_t rx_frame = data->data.as_uint();
        uint8_t tx_frame = m_ctrl->mock_transfer(rx_frame);
        data->data = tx_frame;
        return true;
    }

    return false;
}


uint8_t ArchAVR_SPI::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    if (addr == m_config.reg_data) {
        if (m_ctrl->rx_available())
            value = m_ctrl->pop_rx();

        if (m_intflag_accessed) {
            m_intflag_accessed = false;
            m_intflag.clear_flag();
            clear_ioreg(m_config.rb_wcol);
        }
    }

    return value;
}


uint8_t ArchAVR_SPI::ioreg_peek_handler(reg_addr_t addr, uint8_t value)
{
    if (addr == m_config.reg_data)
        value = m_ctrl->peek_rx();

    return value;
}


void ArchAVR_SPI::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    //Writing data to the DATA register with SPE set triggers a transfer.
    if (addr == m_config.reg_data && test_ioreg(m_config.rb_enable)) {
        if (m_ctrl->active()) {
            set_ioreg(m_config.rb_wcol);
        } else {
            m_ctrl->push_tx(data.value);
            m_intflag.clear_flag();
        }
    }

    //Setting the controller mode depending on bits SPE and MSTR
    if (addr == m_config.rb_enable.addr || addr == m_config.rb_mode.addr) {
        ControllerMode m;
        if (!read_ioreg(m_config.rb_enable))
            m = ControllerMode::Mode_Disabled;
        else if (read_ioreg(m_config.rb_mode))
            m = ControllerMode::Mode_Host;
        else
            m = ControllerMode::Mode_Client;

        m_ctrl->set_mode(m);
    }

    //Writing to SPIE
    if (addr == m_config.rb_int_enable.addr)
        m_intflag.update_from_ioreg();

    if ((addr == m_config.rb_wcol.addr && m_config.rb_wcol.extract(data.value)) ||
        (addr == m_config.rb_int_flag.addr && m_config.rb_int_flag.extract(data.value)))
        m_intflag_accessed = true;

    //Modification of the frame rate
    if (addr == m_config.rb_clock.addr || addr == m_config.rb_clock2x.addr)
        update_framerate();

    //Writing to the serial mode bits
    if (addr == m_config.rb_cpol.addr || addr == m_config.rb_cpha.addr || addr == m_config.rb_dord.addr) {
        update_serial_config();
    }
}


void ArchAVR_SPI::update_framerate()
{
    uint8_t clk_setting = read_ioreg(m_config.rb_clock);
    cycle_count_t clk_factor = ClockFactors[clk_setting];

    if (test_ioreg(m_config.rb_clock2x))
        clk_factor >>= 1;

    m_ctrl->set_bit_delay(clk_factor);
}


void ArchAVR_SPI::update_serial_config()
{
    //Configuring the SPI protocol mode
    uint8_t cpol = read_ioreg(m_config.rb_cpol) & 0x01;
    uint8_t cpha = read_ioreg(m_config.rb_cpha) & 0x01;
    SPI::SerialMode m = (SerialMode)((cpol << 1) | cpha);
    m_ctrl->set_serial_mode(m);

    //Configuring the bit order
    m_ctrl->set_bit_order(test_ioreg(m_config.rb_dord) ? MSBFirst : LSBFirst);
}


void ArchAVR_SPI::frame_completed()
{
    logger().dbg("Frame transfer completed.");
    m_intflag.set_flag(m_config.rb_int_flag.mask);
}


void ArchAVR_SPI::host_selected()
{
    logger().dbg("Host selected, switching to client mode.");
    clear_ioreg(m_config.rb_mode);
    m_ctrl->set_mode(Mode_Client);
    m_intflag.set_flag(m_config.rb_int_flag.mask);
}
