/*
 * arch_avr_usi.cpp
 *
 *  Copyright 2025 Clement Savergne <csavergne@yahoo.com>

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

#include "arch_avr_usi.h"
#include "core/sim_pin.h"
#include "core/sim_device.h"
#include "arch_avr_timer.h"

YASIMAVR_USING_NAMESPACE


enum ClockMode {
    Clock_Strobe = 0,
    Clock_TimerCompMatch,
    Clock_ExtPosEdge,
    Clock_ExtNegEdge,
};

enum WireMode {
    Wire_Disabled = 0,
    Wire_ThreeWire,
    Wire_TwoWire,
    Wire_TwoWireHold,
};

enum PinIndex : PinDriver::pin_index_t {
    Pin_Clock = 0,
    Pin_DataOutput,
    Pin_SerialData,
};


inline bool is_twi(int m)
{
    return m == Wire_TwoWire || m == Wire_TwoWireHold;
}


//=======================================================================================

class ArchAVR_USI::_PinDriver : public PinDriver {

public:

    _PinDriver(ArchAVR_USI& per);

    bool line_state(pin_index_t) const;
    void set_line_state(pin_index_t index, bool state);

    void set_clock_hold(bool hold);

    virtual Pin::controls_t override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls) override;
    virtual void digital_state_changed(pin_index_t pin_index, bool dig_state) override;

private:

    ArchAVR_USI& m_peripheral;
    bool m_line_drive_states[3];
    bool m_clock_hold;

};


ArchAVR_USI::_PinDriver::_PinDriver(ArchAVR_USI& per)
:PinDriver(per.id(), 3)
,m_peripheral(per)
,m_line_drive_states{false, false, false}
,m_clock_hold(false)
{}


bool ArchAVR_USI::_PinDriver::line_state(pin_index_t pin_index) const
{
    return m_line_drive_states[pin_index];
}


void ArchAVR_USI::_PinDriver::set_line_state(pin_index_t index, bool state)
{
    if (state == m_line_drive_states[index]) return;
    m_line_drive_states[index] = state;
    update_pin_state(index);
}


void ArchAVR_USI::_PinDriver::set_clock_hold(bool hold)
{
    m_clock_hold = hold;
    update_pin_state(Pin_Clock);
}


Pin::controls_t ArchAVR_USI::_PinDriver::override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls)
{
    Pin::controls_t c = gpio_controls;
    switch (pin_index) {
        case Pin_Clock: {
            if (is_twi(m_peripheral.m_wire_mode)) {
                c.dir = m_line_drive_states[Pin_Clock] && !m_clock_hold;
                c.drive = 0;
            }
            else if (m_peripheral.m_wire_mode == Wire_ThreeWire) {
                c.drive = m_line_drive_states[Pin_Clock];
            }
        } break;

        case Pin_DataOutput: {
            if (!is_twi(m_peripheral.m_wire_mode))
                c.drive = m_line_drive_states[Pin_DataOutput];
        } break;

        case Pin_SerialData: {
            if (is_twi(m_peripheral.m_wire_mode)) {
                c.dir &= !m_line_drive_states[Pin_SerialData] | !c.drive;
                c.drive = 0;
            }
        } break;
    }

    return c;
}


void ArchAVR_USI::_PinDriver::digital_state_changed(pin_index_t pin_index, bool dig_state)
{
    if (pin_index == Pin_Clock)
        m_peripheral.clock_state_changed(dig_state);
    else if (pin_index == Pin_SerialData)
        m_peripheral.data_state_changed(dig_state);
}


//=======================================================================================

ArchAVR_USI::ArchAVR_USI(const ArchAVR_USIConfig& config)
:Peripheral(chr_to_id('U', 'S', 'I', '\0'))
,m_config(config)
,m_clk_mode(Clock_Strobe)
,m_wire_mode(Wire_Disabled)
,m_start_detected(false)
,m_ovf_intflag(true)
,m_start_intflag(true)
,m_timer_hook(*this, &ArchAVR_USI::timer_raised)
{
    m_driver = new _PinDriver(*this);
}


ArchAVR_USI::~ArchAVR_USI()
{
    delete m_driver;
}


bool ArchAVR_USI::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.rb_wiremode);
    add_ioreg(m_config.rb_clk_sel);
    add_ioreg(m_config.rb_clk_strobe);
    add_ioreg(m_config.rb_clk_toggle);
    add_ioreg(m_config.reg_data);
    add_ioreg(m_config.reg_buffer, true);
    add_ioreg(m_config.rb_counter);
    add_ioreg(m_config.rb_ovf_flag);
    add_ioreg(m_config.rb_ovf_inten);
    add_ioreg(m_config.rb_start_flag);
    add_ioreg(m_config.rb_start_inten);
    add_ioreg(m_config.rb_stop_flag);

    status &= m_ovf_intflag.init(device,
                                 m_config.rb_ovf_inten,
                                 m_config.rb_ovf_flag,
                                 m_config.iv_ovf);
    status &= m_start_intflag.init(device,
                                 m_config.rb_start_inten,
                                 m_config.rb_start_flag,
                                 m_config.iv_start);

    Signal* tc_sig = get_signal("TC_0");
    if (tc_sig)
        tc_sig->connect(m_timer_hook);

    device.pin_manager().register_driver(*m_driver);

    return status;
}


void ArchAVR_USI::reset()
{
    m_clk_mode = Clock_Strobe;
    set_wire_mode(Wire_Disabled, true);
    m_start_detected = false;
    m_ovf_intflag.update_from_ioreg();
    m_start_intflag.update_from_ioreg();
}


void ArchAVR_USI::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    if (addr == m_config.rb_wiremode.addr) {
        uint8_t wm = m_config.rb_wiremode.extract(data.value);
        set_wire_mode(wm, false);
    }

    if (addr == m_config.rb_clk_sel.addr) {
        uint8_t cs = m_config.rb_clk_sel.extract(data.value);
        bool old_latched = output_latched();
        m_clk_mode = cs;

        //If the clock mode change is making the latch transparent, reflect the MSB onto
        //the data output
        if (old_latched && !output_latched())
            update_data_output();
    }

    if (addr == m_config.rb_ovf_flag.addr) {
        //Writing one to the bit clears the flag
        if (m_config.rb_ovf_flag.extract(data.value)) {
            m_ovf_intflag.clear_flag();

            //In TWI (Hold) mode, clearing the overflow flag releases the clock hold,
            //provided the Start Condition flag is cleared as well.
            if (m_wire_mode == Wire_TwoWireHold && !test_ioreg(m_config.rb_start_flag))
                m_driver->set_clock_hold(false);
        }
    }

    if (addr == m_config.rb_ovf_inten.addr)
        m_ovf_intflag.update_from_ioreg();

    if (addr == m_config.rb_start_flag.addr) {
        //Writing one to the bit clears the flag
        if (m_config.rb_start_flag.extract(data.value)) {
            m_start_intflag.clear_flag();
            m_start_detected = false;

            //In the TwoWire (hold) mode the clock hold is only released if the overflow flag is cleared
            if (m_wire_mode == Wire_TwoWire ||
               (m_wire_mode == Wire_TwoWireHold && !test_ioreg(m_config.rb_ovf_flag)))
                m_driver->set_clock_hold(false);
        }
    }

    if (addr == m_config.rb_start_inten.addr)
        m_start_intflag.update_from_ioreg();

    if (addr == m_config.rb_stop_flag.addr) {
        if (m_config.rb_stop_flag.extract(data.value))
            clear_ioreg(m_config.rb_stop_flag);
    }

    if (addr == m_config.rb_clk_strobe.addr) {
        if (m_clk_mode == Clock_Strobe && m_config.rb_clk_strobe.extract(data.value)) {
            shift_data();
            update_data_output();
            inc_counter();
            clear_ioreg(m_config.rb_clk_strobe);
        }
    }

    if (addr == m_config.rb_clk_toggle.addr && m_config.rb_clk_toggle.extract(data.value)) {
        //Toggle the clock line
        bool b = m_driver->line_state(Pin_Clock);
        m_driver->set_line_state(Pin_Clock, !b);

        //If external clock is selected, and USICLK is set, clock the counter
        if ((m_clk_mode & 0x02) && test_ioreg(m_config.rb_clk_strobe))
            inc_counter();

        clear_ioreg(m_config.rb_clk_toggle);
    }

    if (addr == m_config.reg_data) {
        //Unless the output latch is enabled, the new MSB is directly reflected on the data output
        if (!output_latched())
            update_data_output();
    }
}


void ArchAVR_USI::timer_raised(const signal_data_t& sigdata, int)
{
    //If the clock mode is not set to Timer, nothing to do
    if (m_clk_mode != Clock_TimerCompMatch) return;

    //Filter events to keep only CompareMatch A
    if (sigdata.sigid != ArchAVR_Timer::Signal_CompMatch || sigdata.index != 0) return;

    //Shift the data by one bit and increment the counter
    shift_data();
    update_data_output();
    inc_counter();
}


void ArchAVR_USI::set_wire_mode(int new_mode, bool force)
{
    if (new_mode == m_wire_mode && !force) return;

    m_wire_mode = new_mode;
    logger().dbg("Mode changed m=%d", (int) m_wire_mode);

    if (new_mode == Wire_Disabled)
        m_driver->set_enabled(false);

    if (!output_latched())
        update_data_output();

    if (!is_twi(new_mode)) {
        m_driver->set_clock_hold(false);
        m_start_detected = false;
    }

    if (new_mode != Wire_Disabled)
        m_driver->set_enabled(true);
}


bool ArchAVR_USI::output_latched() const
{
    bool clock_state = m_driver->pin_state(Pin_Clock).digital_value();
    return (m_clk_mode == Clock_ExtPosEdge && !clock_state) ||
           (m_clk_mode == Clock_ExtNegEdge && clock_state);
}


void ArchAVR_USI::shift_data()
{
    uint8_t data = read_ioreg(m_config.reg_data);
    bool lsb = m_driver->pin_state(Pin_SerialData).digital_value();
    data = ((data << 1) & 0xFE) | (lsb ? 0x01 : 0x00);
    write_ioreg(m_config.reg_data, data);
}


void ArchAVR_USI::update_data_output()
{
    uint8_t data = read_ioreg(m_config.reg_data);
    bool msb = data & 0x80;
    if (m_wire_mode == Wire_ThreeWire)
        m_driver->set_line_state(Pin_DataOutput, msb);
    else if (m_wire_mode != Wire_Disabled)
        m_driver->set_line_state(Pin_SerialData, msb);
}


void ArchAVR_USI::inc_counter()
{
    uint8_t cnt = read_ioreg(m_config.rb_counter);

    //Increment (modulo 16) the counter
    cnt = (cnt + 1) % 16;
    write_ioreg(m_config.rb_counter, cnt);

    //If overflow
    if (!cnt) {
        //Copy the serial data from the shift register to the buffer
        uint8_t d = read_ioreg(m_config.reg_data);
        write_ioreg(m_config.reg_buffer, d);

        //Raise the overflow interrupt flag
        m_ovf_intflag.set_flag();

        //in TWI (Hold) mode, set the clock hold
        if (m_wire_mode == Wire_TwoWireHold)
            m_driver->set_clock_hold(true);
    }
}


void ArchAVR_USI::clock_state_changed(bool dig_state)
{
    if (m_clk_mode == Clock_ExtPosEdge) {
        //If positive edge on clock, shift the data
        if (dig_state)
            shift_data();
        //if negative edge on clock, update data output
        else
            update_data_output();

        //If USICLK is 0, we also clock the counter
        if (!test_ioreg(m_config.rb_clk_strobe))
            inc_counter();
    }
    else if (m_clk_mode == Clock_ExtNegEdge) {
        //If negative edge on clock, shift the data
        if (!dig_state)
            shift_data();
        //if positive edge on clock, update data output
        else
            update_data_output();

        //If USICLK is 0, we also clock the counter
        if (!test_ioreg(m_config.rb_clk_strobe))
            inc_counter();
    }

    //If the start detector is set and clock goes low, activate the
    //clock hold
    if (m_start_detected && !dig_state) {
        m_start_detected = false;
        m_driver->set_clock_hold(true);
    }
}


void ArchAVR_USI::data_state_changed(bool dig_state)
{
    //Check for a start or stop condition in TWI mode
    if (is_twi(m_wire_mode) && m_driver->pin_state(Pin_Clock).digital_value()) {
        if (dig_state) {
            set_ioreg(m_config.rb_stop_flag);
        } else {
            m_start_detected = true;
            m_start_intflag.set_flag();
        }
    }
}
