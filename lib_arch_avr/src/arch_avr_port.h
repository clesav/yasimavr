/*
 * arch_avr_port.h
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

#ifndef __YASIMAVR_AVR_PORT_H__
#define __YASIMAVR_AVR_PORT_H__

#include "arch_avr_globals.h"
#include "ioctrl_common/sim_port.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \brief Configuration structure for ArchAVR_Port
 */
struct ArchAVR_PortConfig {

    char name;
    reg_addr_t reg_port;
    reg_addr_t reg_pin;
    reg_addr_t reg_dir;

};

/**
   \brief Implementation of a GPIO port controller for AVR series
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_Port : public Port {

public:

    explicit ArchAVR_Port(const ArchAVR_PortConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

protected:

    virtual void pin_state_changed(uint8_t num, Pin::State state) override;

private:

    const ArchAVR_PortConfig& m_config;
    uint8_t m_portr_value;
    uint8_t m_ddr_value;

    void update_pin_states(uint8_t new_portr, uint8_t new_ddr);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_PORT_H__
