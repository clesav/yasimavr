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

#include "arch_avr_globals.h"
#include "ioctrl_common/sim_acp.h"
#include "ioctrl_common/sim_adc.h"
#include "ioctrl_common/sim_vref.h"
#include "core/sim_interrupt.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/*
 * Implementation of a Analog Comparator for the AVR series
 * Unsupported features:
 *      - INT modes other than BOTHEDGE
 *      - Timer input capture
 */


struct ArchAVR_ACPConfig {

    struct mux_config_t : base_reg_config_t {
        pin_id_t pin;
    };

    std::vector<mux_config_t> mux_pins;

    pin_id_t pos_pin;
    pin_id_t neg_pin;

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


class AVR_ARCHAVR_PUBLIC_API ArchAVR_ACP : public ACP,
                                           public Peripheral,
                                           public SignalHook {

public:

    ArchAVR_ACP(int num, const ArchAVR_ACPConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    const ArchAVR_ACPConfig& m_config;
    InterruptFlag m_intflag;
    DataSignalMux m_pos_mux;
    DataSignalMux m_neg_mux;
    Signal m_out_signal;
    double m_pos_value;
    double m_neg_value;

    void change_pos_channel();
    void change_neg_channel();
    void update_state();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_ACOMP_H__
