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


#ifndef __YASIMAVR_MEGA0_ACOMP_H__
#define __YASIMAVR_MEGA0_ACOMP_H__

#include "ioctrl_common/sim_acp.h"
#include "ioctrl_common/sim_vref.h"
#include "core/sim_interrupt.h"
#include "core/sim_pin.h"


//=======================================================================================
/*
 * Definition of CTLREQ codes for Timer type A
 */

//Request to obtain the DAC output value
#define AVR_CTLREQ_ACP_GET_DAC         1


//=======================================================================================
/*
 * Implementation of a Analog Comparator for the Mega-0/Mega-1 series
 * Unsupported features:
 *      - Pin output
 */

struct AVR_ArchMega0_ACP_Config {

    std::vector<AVR_IO_ACP::channel_config_t> pos_channels;
    std::vector<AVR_IO_ACP::channel_config_t> neg_channels;

    uint32_t vref_channel;
    reg_addr_t reg_base;
    int_vect_t iv_cmp;

};

class DLL_EXPORT AVR_ArchMega0_ACP : public AVR_IO_ACP,
                                     public AVR_Peripheral,
                                     public AVR_SignalHook {

public:

    AVR_ArchMega0_ACP(int num, const AVR_ArchMega0_ACP_Config& config);

    virtual bool init(AVR_Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(uint16_t req, ctlreq_data_t* data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, AVR_SleepMode mode) override;
    virtual void raised(const signal_data_t& sigdata, uint16_t hooktag) override;

private:

    const AVR_ArchMega0_ACP_Config& m_config;
    AVR_InterruptFlag m_intflag;
    AVR_DataSignal m_signal;
    //Pointer to the VREF signal to obtain ACP voltage reference updates
    AVR_DataSignal* m_vref_signal;
    AVR_DataSignalMux m_pos_mux;
    AVR_DataSignalMux m_neg_mux;
    //Boolean indicating if the peripheral is disabled by the current sleep mode
    bool m_sleeping;
    //Hysteresis value
    double m_hysteresis;

    bool register_channels(AVR_DataSignalMux& mux, const std::vector<channel_config_t>& channels);
    void update_DAC();
    void update_hysteresis();
    void update_output();

};

#endif //__YASIMAVR_MEGA0_ACOMP_H__
