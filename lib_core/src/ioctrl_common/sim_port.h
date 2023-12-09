/*
 * sim_port.h
 *
 *  Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_IO_PORT_H__
#define __YASIMAVR_IO_PORT_H__

#include "../core/sim_peripheral.h"
#include "../core/sim_types.h"
#include "../core/sim_pin.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \brief Generic model for a GPIO port controller

   It implements a GPIO port controller for up to 8 pins.
   The exact number of pins is determined by the device configuration.

   At initialization, the port will lookup all possible ports with the letter
   (e.g. port 'A' will lookup and control all pins named 'PAx' (x=0 to 7))

   CTLREQs supported:
    - AVR_CTLREQ_GET_SIGNAL

   Signals :
      Id  |  Index  |  Trigger                          |  Data
      ----|---------|-----------------------------------|-----------------
      0   | -       |  Digital state change by any pin  |  port IN value
 */
class AVR_CORE_PUBLIC_API Port : public Peripheral, public SignalHook {

public:

    explicit Port(char name);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

protected:

    uint8_t pin_mask() const;
    Pin* pin(uint8_t num) const;
    void set_pin_internal_state(uint8_t num, Pin::State state);

    virtual void pin_state_changed(uint8_t num, Pin::State state);

private:

    const char m_name;
    uint8_t m_pinmask;
    Signal m_signal;
    std::vector<Pin*> m_pins;
    uint8_t m_port_value;

};

/// Returns the pin mask, containing a '1' for each existing pin
inline uint8_t Port::pin_mask() const
{
    return m_pinmask;
}


inline Pin* Port::pin(uint8_t num) const
{
    return m_pins[num];
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_IO_PORT_H__
