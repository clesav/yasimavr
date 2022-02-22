/*
 * arch_avr_port.h
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_AVR_TIMER_H__
#define __YASIMAVR_AVR_TIMER_H__

#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "core/sim_signal.h"
#include "ioctrl_common/sim_timer.h"


//=======================================================================================
/*
 * Implementation of a 8bits/16bits Timer/Counter for AVR series
 * Only the Normal and CTC modes are currently implemented
 * Other unsupported features:
 *		- External event capture
 *		- External clock sources
 *		- Compare/capture output on pin
 *      - Asynchronous operations
 */

struct AVR_ArchAVR_TimerConfig {

	enum ClockSource {
		ClockDisabled = 0,
		INT_IO_CLOCK,
	};

	struct clock_config_t : base_reg_config_t {
		ClockSource source;		//Clock source
		uint32_t div;			//Prescaler factor
	};

	enum Mode {
		MODE_INVALID = 0,
		MODE_NORMAL,
		MODE_CTC,
	};

	struct mode_config_t : base_reg_config_t {
		Mode mode;
	};

	struct int_config_t {
		int_vect_t vector;
		uint8_t bit;
	};

	bool is_16bits;

	std::vector<clock_config_t> clocks;		//List of clock source configurations
	std::vector<mode_config_t> modes;		//List of the timer mode configurations
	regbit_t rb_clock;						//Clock/prescaler configuration register
	regbit_t rb_mode;						//Timer mode control register
	regbit_t rb_mode_ext;					//Timer mode control register extension
	reg_addr_t reg_cnt;						//Counter register
	reg_addr_t reg_ocra;					//Output Compare A register
	reg_addr_t reg_ocrb;					//Output Compare B register
	reg_addr_t reg_int_enable;				//Interrupt enable register
	reg_addr_t reg_int_flag;				//Interrupt flag register

	int_config_t int_ovf;					//Overflow Interrupt configuration
	int_config_t int_ocra;
	int_config_t int_ocrb;

};

#define DEF_CLOCK_SOURCE_DISABLED  AVR_ArchAVR_TimerConfig::ClockDisabled, 0
#define DEF_CLOCK_SOURCE_INT(_div) AVR_ArchAVR_TimerConfig::INT_IO_CLOCK , _div

class DLL_EXPORT AVR_ArchAVR_Timer : public AVR_Peripheral, public AVR_SignalHook {

public:

	//The Timer module defines one signal (index 0) that is raised when a Interrupt Flag is set
	//The index of the signal data contains one of the following enum values
	enum SignalId {
		Signal_CompA,
		Signal_CompB,
		Signal_OVF
	};

	AVR_ArchAVR_Timer(int num, const AVR_ArchAVR_TimerConfig& config);

	virtual bool init(AVR_Device& device) override;
	virtual void reset() override;
	virtual bool ctlreq(uint16_t req, ctlreq_data_t *data) override;
	virtual void ioreg_read_handler(reg_addr_t addr) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

	virtual void raised(const signal_data_t& data, uint16_t sigid) override;

private:

    class TimerInterruptHook;
    friend class TimerInterruptHook;

    const AVR_ArchAVR_TimerConfig& m_config;

    //***** Clock management *****
    uint16_t m_clk_ps_max;				//Max value counted by the clock prescaler
    bool m_clk_enabled;

    //***** Counter management *****
    uint16_t m_cnt;							//Current counter value
    uint16_t m_ocra;
    uint16_t m_ocrb;
    uint8_t m_temp;

    //***** Event management
    AVR_PrescaledTimer m_timer;
    uint8_t m_next_event_type;

    //***** Interrupt and signal management *****
    AVR_InterruptFlag m_intflag_ovf;
    AVR_InterruptFlag m_intflag_ocra;
    AVR_InterruptFlag m_intflag_ocrb;
    AVR_Signal m_intflag_signal;

    uint32_t delay_to_event();

    inline uint8_t mode() const
    {
    	return read_ioreg(m_config.rb_mode) | (read_ioreg(m_config.rb_mode_ext) << m_config.rb_mode.bitcount());
    }

};

#endif //__YASIMAVR_AVR_TIMER_H__
