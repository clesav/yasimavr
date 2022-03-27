/*
 * arch_mega0_misc.cpp
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

#include "arch_mega0_misc.h"
#include "arch_mega0_io.h"
#include "arch_mega0_io_utils.h"
#include "core/sim_device.h"


#define VREF_REG_ADDR(reg) \
	(m_base_reg + offsetof(VREF_t, reg))


AVR_ArchMega0_VREF::AVR_ArchMega0_VREF(const reg_addr_t base)
:m_base_reg(base)
{}

bool AVR_ArchMega0_VREF::init(AVR_Device& device)
{
	bool status = AVR_IO_VREF::init(device);

	add_ioreg(VREF_REG_ADDR(CTRLA), VREF_AC0REFSEL_gm | VREF_ADC0REFSEL_gm);
	add_ioreg(VREF_REG_ADDR(CTRLB), VREF_AC0REFEN_bm | VREF_ADC0REFEN_bm);

	return status;
}

double AVR_ArchMega0_VREF::get_reference(User user) const
{
	bitmask_t bm;
	if (user == User_ADC)
		bm = DEF_BITMASK_F(VREF_ADC0REFSEL);
	else
		bm = DEF_BITMASK_F(VREF_AC0REFSEL);
	
	uint8_t sel_value = read_ioreg(VREF_REG_ADDR(CTRLA), bm);
	
	switch(sel_value) {
		case 0 : return 0.55;
		case 1 : return 1.1;
		case 2 : return 2.5;
		case 3 : return 4.3;
		case 4 : return 1.5;
		case 7 : return vcc();
		default : return 0.0;
	}
}

//=======================================================================================
enum InterruptPriority {
	IntrPriorityLevel0 = 0,
	IntrPriorityLevel1 = 1
};

#define INT_REG_ADDR(reg) \
	(m_config.reg_base + offsetof(CPUINT_t, reg))

AVR_ArchMega0_IntCtrl::AVR_ArchMega0_IntCtrl(const AVR_ArchMega0_IntCtrl_Config& config)
:AVR_InterruptController(config.vector_count)
,m_config(config)
{}

bool AVR_ArchMega0_IntCtrl::init(AVR_Device& device)
{
	bool status = AVR_InterruptController::init(device);

	//CTRLA not implemented
	add_ioreg(INT_REG_ADDR(STATUS));
	add_ioreg(INT_REG_ADDR(LVL0PRI));
	add_ioreg(INT_REG_ADDR(LVL1VEC));

	return status;
}

void AVR_ArchMega0_IntCtrl::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
	update_irq();
}

void AVR_ArchMega0_IntCtrl::cpu_ack_irq(int_vect_t vector)
{
	//When a interrupt is acked (its routine is about to be executed),
	//set the corresponding priority level flag
	if (vector > 0) {
		uint8_t priority_bit = (vector == read_ioreg(INT_REG_ADDR(LVL1VEC))) ?
								IntrPriorityLevel1 : IntrPriorityLevel0;
		set_ioreg(INT_REG_ADDR(STATUS), priority_bit);
	}

	AVR_InterruptController::cpu_ack_irq(vector);

	set_interrupt_state(vector, IntrState_Raised);
}

int_vect_t AVR_ArchMega0_IntCtrl::get_next_irq() const
{
	uint8_t status_ex = read_ioreg(INT_REG_ADDR(STATUS));

	if (BITSET(status_ex, IntrPriorityLevel1))
		return AVR_INTERRUPT_NONE;

	int lvl1_vector = read_ioreg(INT_REG_ADDR(LVL1VEC));
	if (lvl1_vector > 0 && interrupt_state(lvl1_vector) == IntrState_Raised)
		return lvl1_vector;

	if (BITSET(status_ex, IntrPriorityLevel0))
		return AVR_INTERRUPT_NONE;

	int lvl0_vector = read_ioreg(INT_REG_ADDR(LVL0PRI));
	if (lvl0_vector == 0) {
		for (int_vect_t i = 0; i < intr_count(); ++i) {
			if (interrupt_state(i) == IntrState_Raised)
				return i;
		}
	} else {
		for (unsigned int i = 1; i <= intr_count(); ++i) {
			int_vect_t v = (i + lvl0_vector) % intr_count();
			if (interrupt_state(v) == IntrState_Raised)
				return v;
		}
	}

	return AVR_INTERRUPT_NONE;
}

void AVR_ArchMega0_IntCtrl::cpu_reti()
{
	//The priority level flag must be cleared
	uint8_t status_ex = read_ioreg(INT_REG_ADDR(STATUS));
	if (BITSET(status_ex, IntrPriorityLevel1)) {
		clear_ioreg(INT_REG_ADDR(STATUS), IntrPriorityLevel1);
	} else {
		clear_ioreg(INT_REG_ADDR(STATUS), IntrPriorityLevel0);
	}

	AVR_InterruptController::cpu_reti();
}

//=======================================================================================

#define RST_REG_ADDR(reg) \
	(m_base_reg + offsetof(RSTCTRL_t, reg))

/*
 * Constructor of a reset controller
 */
AVR_ArchMega0_ResetCtrl::AVR_ArchMega0_ResetCtrl(reg_addr_t base)
:AVR_Peripheral(AVR_IOCTL_RST)
,m_base_reg(base)
,m_rst_flags(0)
{}

bool AVR_ArchMega0_ResetCtrl::init(AVR_Device& device)
{
	bool status = AVR_Peripheral::init(device);
	
	add_ioreg(RST_REG_ADDR(RSTFR));
	add_ioreg(RST_REG_ADDR(SWRR));
	
	return status;
}

void AVR_ArchMega0_ResetCtrl::reset()
{
	//Request the value of the reset flags from the device and set the bits of the
	//register RSTFR accordingly
	ctlreq_data_t reqdata;
	if (device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET_FLAG, &reqdata)) {
		unsigned int flags = reqdata.data.as_uint();
		if (flags & AVR_Device::Reset_BOD)
			m_rst_flags |= RSTCTRL_BORF_bm;
		if (flags & AVR_Device::Reset_WDT)
			m_rst_flags |= RSTCTRL_WDRF_bm;
		if (flags & AVR_Device::Reset_Ext)
			m_rst_flags |= RSTCTRL_EXTRF_bm;
		if (flags & AVR_Device::Reset_SW)
			m_rst_flags |= RSTCTRL_SWRF_bm;
		//On a Power On reset, all the other reset flag bits must be cleared
		if (flags & AVR_Device::Reset_PowerOn)
			m_rst_flags = RSTCTRL_PORF_bm;

		write_ioreg(RST_REG_ADDR(RSTFR), m_rst_flags);
	}
}

void AVR_ArchMega0_ResetCtrl::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
	if (addr == RST_REG_ADDR(RSTFR)) {
		//Clears the flags to which '1' is written
		m_rst_flags &= ~data.value;
		write_ioreg(RST_REG_ADDR(RSTFR), m_rst_flags);
	}
	else if (addr == RST_REG_ADDR(SWRR)){
		//Writing a '1' to SWRE bit triggers a software reset
		if (data.value & RSTCTRL_SWRE_bm) {
			ctlreq_data_t reqdata = { .data = AVR_Device::Reset_SW };
			device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET, &reqdata);
		}
	}
}


//=======================================================================================

#define SIGROW_REG_ADDR(reg) \
	(m_config.reg_base_sigrow + offsetof(SIGROW_t, reg))


AVR_ArchMega0_MiscRegCtrl::AVR_ArchMega0_MiscRegCtrl(const AVR_ArchMega0_Misc_Config& config)
:AVR_Peripheral(AVR_ID('M', 'I', 'S', 'C'))
,m_config(config)
{}

bool AVR_ArchMega0_MiscRegCtrl::init(AVR_Device& device)
{
	bool status = AVR_Peripheral::init(device);

	add_ioreg(CCP);

	for (int i = 0; i < m_config.gpior_count; ++i) {
		add_ioreg(m_config.reg_base_gpior + i);
	}

	add_ioreg(m_config.reg_revid, 0xFF, true);

	add_ioreg(SIGROW_REG_ADDR(DEVICEID0), 0xFF, true);
	add_ioreg(SIGROW_REG_ADDR(DEVICEID1), 0xFF, true);
	add_ioreg(SIGROW_REG_ADDR(DEVICEID2), 0xFF, true);

	for (int i = 0; i < 10; ++i)
		add_ioreg(SIGROW_REG_ADDR(SERNUM0) + i, 0xFF, true);

	return status;
}

void AVR_ArchMega0_MiscRegCtrl::reset()
{
	write_ioreg(m_config.reg_revid, MCU_REVID);
	write_ioreg(SIGROW_REG_ADDR(DEVICEID0), m_config.dev_id & 0xFF);
	write_ioreg(SIGROW_REG_ADDR(DEVICEID1), (m_config.dev_id >> 8) & 0xFF);
	write_ioreg(SIGROW_REG_ADDR(DEVICEID2), (m_config.dev_id >> 16) & 0xFF);
}

bool AVR_ArchMega0_MiscRegCtrl::ctlreq(uint16_t req, ctlreq_data_t* data)
{
	uint8_t* p = (uint8_t*) data->data.as_ptr();
	if (req == AVR_CTLREQ_SET_SERID) {
		for (int i = 0; i < 10; ++i)
			write_ioreg(SIGROW_REG_ADDR(SERNUM0) + i, p[i]);

		return true;
	}
	return false;
}

void AVR_ArchMega0_MiscRegCtrl::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
	if (addr == CCP) {
		if (data.value == CCP_SPM_gc || data.value == CCP_IOREG_gc) {
			const char* mode_name = (data.value == CCP_SPM_gc) ? "SPM" : "IOREG";
			DEBUG_LOG(device()->logger(), "Configuration Control Protection inhibited, mode = %s", mode_name);
			write_ioreg(addr, 0x00);
		}
	}
}
