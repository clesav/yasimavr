/*
 * arch_xt_dac.h
 *
 *  Copyright 2024 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_XT_DAC_H__
#define __YASIMAVR_XT_DAC_H__

#include "arch_xt_globals.h"
#include "core/sim_peripheral.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \brief Configuration structure for ArchXT_DAC
 */
struct ArchXT_DACConfig {

    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Channel index for the internal voltage reference
    unsigned int vref_channel;

};

/**
   \brief Implementation of an Digital-to-Analog Converter for XT core series

   Limitations:
    - No Pin output

   CTLREQs supported:
    - AVR_CTLREQ_GET_SIGNAL : returns a pointer to the instance signal
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_DAC : public Peripheral {

public:

    enum SignalId {
        Signal_Output
    };

    ArchXT_DAC(int num, const ArchXT_DACConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset(int flags) override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, SleepMode mode) override;

private:

    const ArchXT_DACConfig& m_config;
    DataSignal m_signal;
    //Pointer to the VREF signal to obtain voltage reference updates
    DataSignal* m_vref_signal;
    BoundFunctionSignalHook<ArchXT_DAC> m_vref_hook;
    bool m_sleeping;

    void update_output();
    void vref_sig_raised(const signal_data_t& sigdata, int hooktag);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_DAC_H__
