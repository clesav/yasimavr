/*
 * arch_avr_acp.h
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

#ifndef __YASIMAVR_AVR_ACOMP_H__
#define __YASIMAVR_AVR_ACOMP_H__

#include "ioctrl_common/sim_acp.h"
#include "ioctrl_common/sim_adc.h"
#include "ioctrl_common/sim_vref.h"
#include "core/sim_interrupt.h"


//=======================================================================================
/*
 * Implementation of a Analog Comparator for the AVR series
 * Unsupported features:
 *      - INT modes other than BOTHEDGE
 *      - Timer input capture
 */


struct AVR_ArchAVR_ACP_Config {

    struct mux_config_t : base_reg_config_t {
        uint32_t pin;
    };

    std::vector<mux_config_t> mux_pins;

    uint32_t pos_pin;
    uint32_t neg_pin;

    regbit_t rb_disable;
    regbit_t rb_mux_enable;
    regbit_t rb_adc_enable;
    regbit_t rb_mux;
    regbit_t rb_bandgap_select;
    regbit_t rb_int_mode;
    regbit_t rb_output;
    regbit_t rb_int_enable;
    regbit_t rb_int_flag;

    int_vect_t iv_cmp;
};


class DLL_EXPORT AVR_ArchAVR_ACP : public AVR_IO_ACP,
                                   public AVR_Peripheral,
                                   public AVR_SignalHook {

public:

    AVR_ArchAVR_ACP(int num, const AVR_ArchAVR_ACP_Config& config);

    virtual bool init(AVR_Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(uint16_t req, ctlreq_data_t* data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void raised(const signal_data_t& sigdata, uint16_t hooktag) override;

private:

    const AVR_ArchAVR_ACP_Config& m_config;
    AVR_InterruptFlag m_intflag;
    AVR_DataSignalMux m_pos_mux;
    AVR_DataSignalMux m_neg_mux;
    AVR_Signal m_out_signal;
    double m_pos_value;
    double m_neg_value;

    void change_pos_channel();
    void change_neg_channel();
    void update_state();

};

#endif //__YASIMAVR_AVR_ACOMP_H__
