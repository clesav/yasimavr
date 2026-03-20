/*
 * arch_xt_port.cpp
 *
 *  Copyright 2022-2026 Clement Savergne <csavergne@yahoo.com>

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

#include "arch_xt_port.h"
#include "arch_xt_device.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define PORT_REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base_port + offsetof(PORT_t, reg))

#define PORT_REG_OFS(reg) \
    offsetof(PORT_t, reg)

#define VPORT_REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base_vport + offsetof(VPORT_t, reg))

#define VPORT_REG_OFS(reg) \
    offsetof(VPORT_t, reg)


//=======================================================================================

class ArchXT_Port::_InterruptHandler : public AbstractInterruptFlag {

public:

    _InterruptHandler()
    :AbstractInterruptFlag()
    ,m_int_enable(0x00)
    ,m_intflag_reg(nullptr)
    {}

    bool init(Device& device, const reg_addr_t& addr_intflag, int_vect_t vector)
    {
        m_intflag_reg = device.core().get_ioreg(addr_intflag);
        return m_intflag_reg && AbstractInterruptFlag::init(device, vector);
    }

    void reset()
    {
        m_int_enable = 0x00;
    }

    void set_pin_int_enabled(uint8_t pin, bool enabled)
    {
        if (enabled)
            m_int_enable |= 1 << pin;
        else
            m_int_enable &= ~(1 << pin);

        update();
    }

private:

    uint8_t m_int_enable;
    IORegister* m_intflag_reg;

    virtual bool flag_raised() const override
    {
        uint8_t intflags = m_intflag_reg->value();
        return intflags & m_int_enable;
    }

};


//=======================================================================================

ArchXT_Port::ArchXT_Port(char name, const ArchXT_PortConfig& config)
:Port(name)
,m_config(config)
{
    m_int_handler = new _InterruptHandler();
}


ArchXT_Port::~ArchXT_Port()
{
    delete m_int_handler;
}


bool ArchXT_Port::init(Device& device)
{
    bool status = Port::init(device);

    bitmask_t pm = pin_mask();

    add_ioreg(PORT_REG_ADDR(DIR), pm);
    add_ioreg(PORT_REG_ADDR(DIRSET), pm, IORegister::Strobe);
    add_ioreg(PORT_REG_ADDR(DIRCLR), pm, IORegister::Strobe);
    add_ioreg(PORT_REG_ADDR(DIRTGL), pm, IORegister::Strobe);
    add_ioreg(PORT_REG_ADDR(OUT), pm);
    add_ioreg(PORT_REG_ADDR(OUTSET), pm, IORegister::Strobe);
    add_ioreg(PORT_REG_ADDR(OUTCLR), pm, IORegister::Strobe);
    add_ioreg(PORT_REG_ADDR(OUTTGL), pm, IORegister::Strobe);
    add_ioreg(PORT_REG_ADDR(IN), pm);
    add_ioreg(PORT_REG_ADDR(INTFLAGS), pm);
    add_ioreg(PORT_REG_ADDR(PORTCTRL), PORT_SRL_bm);
    for (uint8_t i = 0; i < 8; ++i) {
        if (pm.bit(i))
            add_ioreg(PORT_REG_ADDR(PIN0CTRL) + i, PORT_ISC_gm | PORT_PULLUPEN_bm | PORT_INVEN_bm);
    }

    //Virtual port registers
    add_ioreg(VPORT_REG_ADDR(DIR), pm);
    add_ioreg(VPORT_REG_ADDR(OUT), pm);
    add_ioreg(VPORT_REG_ADDR(IN), pm);
    add_ioreg(VPORT_REG_ADDR(INTFLAGS), pm);

    m_int_handler->init(device, PORT_REG_ADDR(INTFLAGS), m_config.iv_port);

    return status;
}


uint8_t ArchXT_Port::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    if (addr == VPORT_REG_ADDR(DIR))
        value = read_ioreg(PORT_REG_ADDR(DIR));
    else if (addr == VPORT_REG_ADDR(OUT))
        value = read_ioreg(PORT_REG_ADDR(OUT));
    else if (addr == VPORT_REG_ADDR(IN))
        value = read_ioreg(PORT_REG_ADDR(IN));
    else if (addr == VPORT_REG_ADDR(INTFLAGS))
        value = read_ioreg(PORT_REG_ADDR(INTFLAGS));

    else if (addr == PORT_REG_ADDR(DIRSET))
        value = read_ioreg(PORT_REG_ADDR(DIR));
    else if (addr == PORT_REG_ADDR(DIRCLR))
        value = read_ioreg(PORT_REG_ADDR(DIR));
    else if (addr == PORT_REG_ADDR(DIRTGL))
        value = read_ioreg(PORT_REG_ADDR(DIR));

    else if (addr == PORT_REG_ADDR(OUTSET))
        value = read_ioreg(PORT_REG_ADDR(OUT));
    else if (addr == PORT_REG_ADDR(OUTCLR))
        value = read_ioreg(PORT_REG_ADDR(OUT));
    else if (addr == PORT_REG_ADDR(OUTTGL))
        value = read_ioreg(PORT_REG_ADDR(OUT));

    return value;
}


void ArchXT_Port::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    if (addr >= m_config.reg_base_port && addr < reg_addr_t(m_config.reg_base_port + sizeof(PORT_t))) {
        reg_addr_t reg_ofs = addr - m_config.reg_base_port;
        uint8_t value_masked = data.value & pin_mask();
        switch(reg_ofs) {
        case PORT_REG_OFS(DIR): {
            write_ioreg(addr, value_masked);
            update_pin_states();
        } break;

        case PORT_REG_OFS(DIRSET): {
            uint8_t dir_value = read_ioreg(PORT_REG_ADDR(DIR)) | value_masked;
            write_ioreg(PORT_REG_ADDR(DIR), dir_value);
            update_pin_states();
        } break;

        case PORT_REG_OFS(DIRCLR): {
            uint8_t dir_value = read_ioreg(PORT_REG_ADDR(DIR)) & ~value_masked;
            write_ioreg(PORT_REG_ADDR(DIR), dir_value);
            update_pin_states();
        } break;

        case PORT_REG_OFS(DIRTGL): {
            uint8_t dir_value = read_ioreg(PORT_REG_ADDR(DIR)) ^ value_masked;
            write_ioreg(PORT_REG_ADDR(DIR), dir_value);
            update_pin_states();
        } break;

        case PORT_REG_OFS(OUT): {
            write_ioreg(PORT_REG_ADDR(OUT), value_masked);
            update_pin_states();
        } break;

        case PORT_REG_OFS(OUTSET): {
            uint8_t out_value = read_ioreg(PORT_REG_ADDR(OUT)) | value_masked;
            write_ioreg(PORT_REG_ADDR(OUT), out_value);
            update_pin_states();
        } break;

        case PORT_REG_OFS(OUTCLR): {
            uint8_t out_value = read_ioreg(PORT_REG_ADDR(OUT)) & ~value_masked;
            write_ioreg(PORT_REG_ADDR(OUT), out_value);
            update_pin_states();
        } break;

        case PORT_REG_OFS(OUTTGL):
        case PORT_REG_OFS(IN): {
            uint8_t out_value = read_ioreg(PORT_REG_ADDR(OUT)) ^ value_masked;
            write_ioreg(PORT_REG_ADDR(OUT), out_value);
            update_pin_states();
        } break;

        case PORT_REG_OFS(INTFLAGS): {
            uint8_t new_intflags = data.old & ~data.value;
            //For pins with ISC==LEVEL, the flag cannot be cleared if the input is low.
            for (uint8_t i = 0; i < 8; ++i) {
                if (!pin_mask().bit(i)) continue;
                if ((read_ioreg(PORT_REG_ADDR(PIN0CTRL) + i) & PORT_ISC_gm) != PORT_ISC_LEVEL_gc) continue;
                if (read_ioreg(PORT_REG_ADDR(IN), bitspec_t(i))) continue;
                new_intflags |= 1 << i;
            }
            write_ioreg(addr, new_intflags);
            m_int_handler->update();
        } break;

        case PORT_REG_OFS(PIN0CTRL):
        case PORT_REG_OFS(PIN1CTRL):
        case PORT_REG_OFS(PIN2CTRL):
        case PORT_REG_OFS(PIN3CTRL):
        case PORT_REG_OFS(PIN4CTRL):
        case PORT_REG_OFS(PIN5CTRL):
        case PORT_REG_OFS(PIN6CTRL):
        case PORT_REG_OFS(PIN7CTRL): {
            uint8_t bitnum = reg_ofs - PORT_REG_OFS(PIN0CTRL);
            if (pin_mask().bit(bitnum)) {
                uint8_t isc = data.value & DEF_BITSPEC_F(PORT_ISC);
                bool int_enable = isc != PORT_ISC_INTDISABLE_gc && isc != PORT_ISC_INPUT_DISABLE_gc;
                m_int_handler->set_pin_int_enabled(bitnum, int_enable);
                //Update the pin states to take into account any change of PULLUPEN
                update_pin_states();
                //Force an update of the input to take into account any change of INVEN
                update_input(bitnum);
            }
        } break;

        default: break;
        }

    } else {

        reg_addr_t reg_ofs = addr - m_config.reg_base_vport;
        switch(reg_ofs) {
        case VPORT_REG_OFS(DIR):
            ioreg_write_handler(PORT_REG_ADDR(DIR), data);
            break;

        case VPORT_REG_OFS(OUT):
            ioreg_write_handler(PORT_REG_ADDR(OUT), data);
            break;

        case VPORT_REG_OFS(IN):
            ioreg_write_handler(PORT_REG_ADDR(OUTTGL), data);
            break;

        case VPORT_REG_OFS(INTFLAGS):
            ioreg_write_handler(PORT_REG_ADDR(INTFLAGS), data);
            break;

        default: break;
        }
    }
}


void ArchXT_Port::update_pin_states()
{
    Pin::controls_t controls;
    uint8_t valdir = read_ioreg(PORT_REG_ADDR(DIR));
    uint8_t valport = read_ioreg(PORT_REG_ADDR(OUT));
    uint8_t pinmask = pin_mask().mask;

    for (int i = 0; i < 8; ++i) {
        if (pinmask & 0x01) {
            uint8_t pin_cfg = read_ioreg(PORT_REG_ADDR(PIN0CTRL) + i);
            controls.dir = valdir & 1;
            controls.drive = valport & 1;
            controls.pull_up = pin_cfg & PORT_PULLUPEN_bm;
            controls.inverted = pin_cfg & PORT_INVEN_bm;
            set_pin_internal_state(i, controls);
        }

        valdir >>= 1;
        valport >>= 1;
        pinmask >>= 1;
    }
}


void ArchXT_Port::pin_state_changed(uint8_t num, Wire::StateEnum state)
{
    Port::pin_state_changed(num, state);
    if (state == Pin::State_Shorted) return;
    update_input(num);
}


void ArchXT_Port::update_input(uint8_t num)
{
    //Extract the current pin boolean state
    bitspec_t pin_bs = bitspec_t(num);
    bool curr_value = test_ioreg(PORT_REG_ADDR(IN), pin_bs);

    //Read the pin configuration
    uint8_t pin_cfg = read_ioreg(PORT_REG_ADDR(PIN0CTRL) + num);

    //Extract the new pin boolean state and invert if enabled
    bool new_value = pin(num)->digital_state();
    if (pin_cfg & DEF_BITSPEC_B(PORT_INVEN))
        new_value = !new_value;

    //if there's no change, we return
    if (new_value == curr_value) return;

    //Read the Input Sense Config field of the pin control register
    uint8_t isc = pin_cfg & DEF_BITSPEC_F(PORT_ISC);

    //If ISC is Input Disabled, we can ignore the change
    if (isc == PORT_ISC_INPUT_DISABLE_gc) return;

    //Save the new pin state in the IN register
    write_ioreg(PORT_REG_ADDR(IN), pin_bs, new_value);

    //Check if we need to raise the interrupt
    bool raise_int = (isc == PORT_ISC_RISING_gc && new_value) ||
                     (isc == PORT_ISC_FALLING_gc && !new_value) ||
                     (isc == PORT_ISC_LEVEL_gc && !new_value) ||
                     (isc == PORT_ISC_BOTHEDGES_gc);

    if (raise_int) {
        set_ioreg(PORT_REG_ADDR(INTFLAGS), pin_bs);
        m_int_handler->update();
    }
}
