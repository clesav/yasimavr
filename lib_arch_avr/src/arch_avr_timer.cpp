/*
 * arch_avr_port.cpp
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

#include "arch_avr_timer.h"
#include "core/sim_device.h"

static const char* ClockSourceNames[2] = {
	"Disabled",
	"Internal I/O"
};

#define CLOCKSOURCENAME(ix) ClockSourceNames[m_config.clocks[ix].source]
#define CLOCKPRESCALER(ix) (1 << (m_config.clocks[ix].div))


AVR_ArchAVR_Timer::AVR_ArchAVR_Timer(int num, const AVR_ArchAVR_TimerConfig& config)
:AVR_Peripheral(AVR_IOCTL_TIMER('_', 0x30 + num))
,m_config(config)
,m_clk_enabled(false)
,m_cnt(0)
,m_ocra(0)
,m_ocrb(0)
,m_temp(0)
,m_next_event_type(0)
,m_intflag_ovf(true)
,m_intflag_ocra(true)
,m_intflag_ocrb(true)
{
	//Calculate the prescaler max value by looking for the highest division factor
	unsigned int maxdiv = 0;
	for (auto clkcfg : m_config.clocks) {
		if (clkcfg.div > maxdiv)
			maxdiv = clkcfg.div;
	}
	m_clk_ps_max = 1 << maxdiv;
}

bool AVR_ArchAVR_Timer::init(AVR_Device& device)
{
	bool status = AVR_Peripheral::init(device);

	add_ioreg(m_config.rb_clock.addr);
	add_ioreg(m_config.rb_mode);
	add_ioreg(m_config.rb_mode_ext);
	add_ioreg(m_config.reg_cnt);
	add_ioreg(m_config.reg_ocra);
	add_ioreg(m_config.reg_ocrb);

	if (m_config.is_16bits) {
		device.add_ioreg_handler(m_config.reg_cnt + 1, this);
		device.add_ioreg_handler(m_config.reg_ocra + 1, this);
		device.add_ioreg_handler(m_config.reg_ocrb + 1, this);
	}

	uint8_t int_bitmask = (1 << m_config.int_ovf.bit) |
					  	  (1 << m_config.int_ocra.bit) |
						  (1 << m_config.int_ocrb.bit);
	add_ioreg(m_config.reg_int_enable, int_bitmask);
	add_ioreg(m_config.reg_int_flag, int_bitmask);

	m_intflag_ovf.init(device,
				   	   regbit_t(m_config.reg_int_enable, m_config.int_ovf.bit),
					   regbit_t(m_config.reg_int_flag, m_config.int_ovf.bit),
					   m_config.int_ovf.vector);
	m_intflag_ocra.init(device,
				   		regbit_t(m_config.reg_int_enable, m_config.int_ocra.bit),
					    regbit_t(m_config.reg_int_flag, m_config.int_ocra.bit),
					    m_config.int_ocra.vector);
	m_intflag_ocrb.init(device,
				   		regbit_t(m_config.reg_int_enable, m_config.int_ocrb.bit),
					    regbit_t(m_config.reg_int_flag, m_config.int_ocrb.bit),
					    m_config.int_ocrb.vector);

	m_timer.init(device.cycle_manager(), device.logger());
	m_timer.signal().connect_hook(this);

	return status;
}

void AVR_ArchAVR_Timer::reset()
{
	m_cnt = 0;
	m_ocra = 0;
	m_ocrb = 0;
	m_temp = 0;
	m_next_event_type = 0;
	m_timer.reset();
	m_timer.set_prescaler(m_clk_ps_max, 1);
}

bool AVR_ArchAVR_Timer::ctlreq(uint16_t req, ctlreq_data_t *data)
{
	if (req == AVR_CTLREQ_GET_SIGNAL) {
		data->p = &m_intflag_signal;
		return true;
	}
	return false;
}

void AVR_ArchAVR_Timer::ioreg_read_handler(reg_addr_t addr)
{
	//reading of interrupt flags
	if (addr == m_config.reg_int_flag)
		m_timer.update(device()->cycle());

	//8 or 16 bits reading of CNTx
	else if (addr == m_config.reg_cnt) {
		m_timer.update(device()->cycle());
		write_ioreg(m_config.reg_cnt, m_cnt & 0x00FF);
		if (m_config.is_16bits)
			m_temp = m_cnt >> 8;
	}
	else if (m_config.is_16bits && addr == m_config.reg_cnt + 1)
		write_ioreg(m_config.reg_cnt + 1, m_temp);

	//8 or 16 bits reading of OCRxA
	else if (addr == m_config.reg_ocra)
		write_ioreg(m_config.reg_ocra, m_ocra & 0x00FF);
	else if (m_config.is_16bits && addr == m_config.reg_ocra + 1)
		write_ioreg(m_config.reg_ocra + 1, m_ocra >> 8);

	//8 or 16 bits reading of OCRxB
	else if (addr == m_config.reg_ocrb)
		write_ioreg(m_config.reg_ocrb, m_ocrb & 0x00FF);
	else if (m_config.is_16bits && addr == m_config.reg_ocrb + 1)
		write_ioreg(m_config.reg_ocrb + 1, m_ocrb >> 8);
}

void AVR_ArchAVR_Timer::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
	bool do_reschedule = false;

	//8 or 16 bits writing to CNTx
	if (addr == m_config.reg_cnt) {
		m_cnt = (m_temp << 8) | data.value;
		do_reschedule = true;
	}
	else if (m_config.is_16bits && addr == m_config.reg_cnt + 1) {
		m_temp = data.value;
	}
	//8 or 16 bits writing to OCRxA
	else if (addr == m_config.reg_ocra) {
		m_ocra = (m_temp << 8) | data.value;
		do_reschedule = true;
	}
	else if (m_config.is_16bits && addr == m_config.reg_ocra + 1) {
		m_temp = data.value;
	}
	//8 or 16 bits writing to OCRxB
	else if (addr == m_config.reg_ocrb) {
		m_ocrb = (m_temp << 8) | data.value;
		do_reschedule = true;
	}
	else if (m_config.is_16bits && addr == m_config.reg_ocrb + 1) {
		m_temp = data.value;
	}

	else {
		if (addr == m_config.rb_clock.addr) {
			uint32_t ps_factor;
			uint8_t v = m_config.rb_clock.extract(data.value);
			int clock_index = find_reg_config<AVR_ArchAVR_TimerConfig::clock_config_t>(m_config.clocks, v);
			if (clock_index >= 0) {
				DEBUG_LOG(device()->logger(), "Timer %s clock changed to SRC=%s, PS=%d",
											   name().c_str(),
											   CLOCKSOURCENAME(clock_index),
											   CLOCKPRESCALER(clock_index));
				if (m_config.clocks[clock_index].source == AVR_ArchAVR_TimerConfig::ClockDisabled) {
					ps_factor = 1;
					m_clk_enabled = false;
				} else {
					ps_factor = m_config.clocks[clock_index].div;
					m_clk_enabled = true;
				}
			} else {
				DEBUG_LOG(device()->logger(), "Timer %s clock changed to unsupported mode: 0x%02x", v);
				ps_factor = 1;
				m_clk_enabled = false;
			}

			m_timer.set_prescaler(m_clk_ps_max, ps_factor);
			do_reschedule = true;
		}

		//If we're writing a 1 to a interrupt flag bit, it cancels the corresponding interrupt if it
		//has not been executed yet
		if (addr == m_config.reg_int_flag) {
			if (m_config.int_ovf.vector && BITSET(data.value, m_config.int_ovf.bit))
				if (m_intflag_ovf.clear_flag())
					m_intflag_signal.raise_u(Signal_OVF, 0, 0);

			if (m_config.int_ocra.vector && BITSET(data.value, m_config.int_ocra.bit))
				if (m_intflag_ocra.clear_flag())
					m_intflag_signal.raise_u(Signal_CompA, 0, 0);

			if (m_config.int_ocrb.vector && BITSET(data.value, m_config.int_ocrb.bit))
				if (m_intflag_ocrb.clear_flag())
					m_intflag_signal.raise_u(Signal_CompB, 0, 0);
		}
	}

	if (do_reschedule) {
		DEBUG_LOG(device()->logger(), "Rescheduling %s", name().c_str());
		m_timer.set_timer_delay(m_clk_enabled ? delay_to_event() : 0);
	}
}

/*
 * Defines the types of 'event' that can be triggered by the counter
 */
enum TimerEventType {
	TimerEventMax		= 0x01,
	TimerEventCompA		= 0x02,
	TimerEventCompB		= 0x04
};

/*
 * Calculates the delay in prescaler ticks and the type of the next timer/counter event
 */
uint32_t AVR_ArchAVR_Timer::delay_to_event()
{
	int ticks_to_max = (m_config.is_16bits ? 0x10000 : 0x100) - (int)m_cnt;
	int ticks_to_ocra = (int)m_ocra - (int)m_cnt + 1;
	int ticks_to_ocrb = (int)m_ocrb - (int)m_cnt + 1;

	int ticks_to_next_event = ticks_to_max;

	if (ticks_to_ocra > 0 && ticks_to_ocra < ticks_to_next_event)
		ticks_to_next_event = ticks_to_ocra;
	if (ticks_to_ocrb > 0 && ticks_to_ocrb < ticks_to_next_event)
		ticks_to_next_event = ticks_to_ocrb;

	m_next_event_type = 0;
	if (ticks_to_next_event == ticks_to_max)
		m_next_event_type |= TimerEventMax;
	if (ticks_to_next_event == ticks_to_ocra)
		m_next_event_type |= TimerEventCompA;
	if (ticks_to_next_event == ticks_to_ocrb)
		m_next_event_type |= TimerEventCompB;

	DEBUG_LOG(device()->logger(), "Next event for %s: 0x%x in %d cycles",
			  name().c_str(), m_next_event_type, ticks_to_next_event);

	return (uint32_t)ticks_to_next_event;
}


void AVR_ArchAVR_Timer::raised(const signal_data_t& data, uint16_t sigid)
{
	DEBUG_LOG(device()->logger(), "Updating counters of %s", name().c_str());

	m_cnt += data.u;

	if (!data.index) return;

	if (m_next_event_type & TimerEventCompA) {
		DEBUG_LOG(device()->logger(), "Timer %s triggering interrupt OCRA", name().c_str());

		if (m_intflag_ocra.set_flag())
			m_intflag_signal.raise_u(Signal_CompA, 0, 1);

		uint8_t timer_mode = mode();
		if (timer_mode == AVR_ArchAVR_TimerConfig::MODE_CTC)
			m_cnt = 0;
	}

	if (m_next_event_type & TimerEventCompB) {
		DEBUG_LOG(device()->logger(), "Timer %s triggering interrupt OCRB", name().c_str());

		if (m_intflag_ocrb.set_flag())
			m_intflag_signal.raise_u(Signal_CompB, 0, 1);
	}

	if (m_next_event_type & TimerEventMax) {
		DEBUG_LOG(device()->logger(), "Timer %s triggering interrupt OVF", name().c_str());

		if (m_intflag_ovf.set_flag())
			m_intflag_signal.raise_u(Signal_OVF, 0, 1);

		m_cnt = 0;
	}

	m_timer.set_timer_delay(delay_to_event());
}
