/*
 * arch_xt_acp.h
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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


#ifndef __YASIMAVR_XT_ACOMP_H__
#define __YASIMAVR_XT_ACOMP_H__

#include "arch_xt_globals.h"
#include "ioctrl_common/sim_acp.h"
#include "ioctrl_common/sim_vref.h"
#include "core/sim_interrupt.h"
#include "core/sim_pin.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   Definition of CTLREQ codes for ACP
 */

//Request to obtain the DAC output value
#define AVR_CTLREQ_ACP_GET_DAC         1


//=======================================================================================

/**
   \brief Configuration structure for ArchXT_ACP
 */
struct ArchXT_ACPConfig {

    /// List of the channels for the positive input
    std::vector<ACP::channel_config_t> pos_channels;
    /// List of the channels for the negative input
    std::vector<ACP::channel_config_t> neg_channels;
    /// Channel index for the internal voltage reference
    unsigned int vref_channel;
    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Interrupt vector index for the comparator output
    int_vect_t iv_cmp;

};

/**
   \brief Implementation of an Analog Comparator for XT core series

   \sa ACP

   Limitations:
    - Pin output
    - No Debug Run override

   CTLREQs supported:
    - AVR_CTLREQ_GET_SIGNAL : returns a pointer to the instance signal
    - AVR_CTLREQ_ACP_GET_DAC : returns the output value of the internal DAC.
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_ACP : public ACP,
                                         public Peripheral,
                                         public SignalHook {

public:

    ArchXT_ACP(int num, const ArchXT_ACPConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, SleepMode mode) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    const ArchXT_ACPConfig& m_config;
    InterruptFlag m_intflag;
    DataSignal m_signal;
    //Pointer to the VREF signal to obtain ACP voltage reference updates
    DataSignal* m_vref_signal;
    DataSignalMux m_pos_mux;
    DataSignalMux m_neg_mux;
    //Boolean indicating if the peripheral is disabled by the current sleep mode
    bool m_sleeping;
    //Hysteresis value
    double m_hysteresis;

    bool register_channels(DataSignalMux& mux, const std::vector<channel_config_t>& channels);
    void update_DAC();
    void update_hysteresis();
    void update_output();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_ACOMP_H__
