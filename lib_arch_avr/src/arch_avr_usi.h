/*
 * arch_avr_usi.h
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

#ifndef __YASIMAVR_AVR_USI_H__
#define __YASIMAVR_AVR_USI_H__

#include "arch_avr_globals.h"
#include "core/sim_interrupt.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \ingroup api_usi
   \brief Configuration structure for ArchAVR_USI
 */
struct ArchAVR_USIConfig {

    regbit_t rb_wiremode;           ///< Wire mode selection
    regbit_t rb_clk_sel;            ///< Clock mode selection
    regbit_t rb_clk_strobe;         ///< Clock strobe
    regbit_t rb_clk_toggle;         ///< Clock toggle
    reg_addr_t reg_data;            ///< Data register
    reg_addr_t reg_buffer;          ///< Data buffer register
    regbit_t rb_counter;            ///< 4-bits counter
    regbit_t rb_ovf_flag;           ///< Overflow flag
    regbit_t rb_ovf_inten;          ///< Overflow interrupt enable
    regbit_t rb_start_flag;         ///< Start condition flag
    regbit_t rb_start_inten;        ///< Start condition interrupt enable
    regbit_t rb_stop_flag;          ///< Stop condition flag

    int_vect_t iv_ovf;              ///< Overflow interrupt vector index
    int_vect_t iv_start;            ///< Start condition interrupt vector index

};


/**
   \ingroup api_usi
   \brief Implementation of a Universal Serial Interface for AVR series

   Unsupported features:
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_USI : public Peripheral {

public:

    explicit ArchAVR_USI(const ArchAVR_USIConfig& config);
    virtual ~ArchAVR_USI();

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    class _PinDriver;
    friend class _PinDriver;

    const ArchAVR_USIConfig& m_config;

    _PinDriver* m_driver;
    int m_clk_mode;
    int m_wire_mode;
    bool m_start_detected;

    InterruptFlag m_ovf_intflag;
    InterruptFlag m_start_intflag;

    DataSignal m_signal;

    BoundFunctionSignalHook<ArchAVR_USI> m_timer_hook;

    void timer_raised(const signal_data_t& sigdata, int hooktag);
    void set_wire_mode(int new_mode, bool force);
    bool output_latched() const;
    void shift_data();
    void update_data_output();
    void inc_counter();
    void clock_state_changed(bool dig_state);
    void data_state_changed(bool dig_state);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_USI_H__
