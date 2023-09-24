/*
 * sim_sleep.h
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

#ifndef __YASIMAVR_SLEEP_H__
#define __YASIMAVR_SLEEP_H__

#include "sim_peripheral.h"
#include "sim_signal.h"
#include "sim_types.h"

YASIMAVR_BEGIN_NAMESPACE

//=======================================================================================
/*
 * CTLREQ definitions
*/

//AVR_CTLREQ_SLEEP_CALL (defined in sim_peripheral.h) :
//Request sent by the CPU when executing a SLEEP instruction.
//No data is required

//AVR_CTLREQ_SLEEP_PSEUDO (defined in sim_peripheral.h) :
//Request sent by the CPU when executing a infinite loop (RJMP .-2) with GIE flag set
//No data is required

/**
 *Definition of generic sleep modes
*/
enum class SleepMode {
    Invalid,
    Active,
    Pseudo,
    Idle,
    ADC,
    Standby,
    ExtStandby,
    PowerDown,
    PowerSave,
};

const char* SleepModeName(SleepMode mode) AVR_CORE_PUBLIC_API;


//=======================================================================================

/**
   \brief Configuration structure for a generic sleep mode controller.
   \sa AVR_SleepController
 */
struct SleepConfig {

    /**
       \brief Configuration structure for each supported sleep mode
       \param reg_value the register field value for this sleep mode
       \param mode the SleepMode enum value corresponding to this sleep mode
       \param int_mask bitset that indicates for each interrupt vector of the device vector map
       if it can wake up the device from this sleep mode
     */
    struct mode_config_t : base_reg_config_t {
        SleepMode mode;
        uint8_t int_mask[16];
    };

    ///The supported sleep modes
    std::vector<mode_config_t> modes;
    ///Register location for the sleep mode selection
    regbit_t rb_mode;
    ///Register location for the sleep enable bit
    regbit_t rb_enable;
};


/**
   \brief Generic sleep mode controller

   It listens for the sleep instruction notification from the CPU and,
   using the register configuration, determines which sleep mode the MCU
   should enter. It then sends a CTL request to the device to enter sleep.

   It is also connected to the interrupt controller to listen to any interrupt
   raise, and determines if the interrupt is able to wake the MCU up.
   If so, it sends a CTL request to the device to exit sleep.

   This controller also manages the "pseudo-sleep" mode, which is treated as any
   other sleep mode, and which can be exited by any interrupt.

   All the configuration is done at construction by mean of a AVR_SleepConfig object.
 */
class AVR_CORE_PUBLIC_API SleepController : public Peripheral, public SignalHook {

public:

    explicit SleepController(const SleepConfig& config);

    virtual bool init(Device& device) override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    const SleepConfig& m_config;
    //Index of the current sleep mode in the configuration mode map
    unsigned int m_mode_index;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_SLEEP_H__
