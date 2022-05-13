/*
 * sim_core.cpp
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

#include "sim_core.h"
#include "sim_ioreg.h"
#include "sim_signal.h"
#include "sim_interrupt.h"
#include "sim_device.h"
#include <cstring>


//=======================================================================================

AVR_Core::AVR_Core(const AVR_CoreConfiguration& config)
:m_config(config)
,m_device(nullptr)
,m_programend(m_config.flashend)
,m_pc(0)
,m_int_inhib_counter(0)
,m_debug_probe(nullptr)
,m_intrctl(nullptr)
{
	//Allocate the SRAM in RAM
	size_t sram_size = m_config.ramend - m_config.ramstart + 1;
	m_sram = (uint8_t*) malloc(sram_size);
	std::memset(m_sram, 0x00, sram_size);

	//Allocate the flash sector in RAM
	size_t flash_size = m_config.flashend + 1;
	m_flash = (uint8_t*) malloc(flash_size);
	std::memset(m_flash, 0xff, flash_size);
	m_flash_tag = (uint8_t*) malloc(flash_size);
	std::memset(m_flash_tag, 0, flash_size);

	//Allocate the I/O registers in RAM
	//We don't actually allocate any register until the peripherals ask for it
	size_t ioreg_size = sizeof(AVR_IO_Register*) * (m_config.ioend - m_config.iostart + 1);
	m_ioregs = (AVR_IO_Register**) malloc(ioreg_size);
	std::memset(m_ioregs, 0, ioreg_size);

	//Create the I/O registers managed by the CPU
	m_ioregs[R_SPL] = new AVR_IO_Register(true);
	m_ioregs[R_SPH] = new AVR_IO_Register(true);

	//If extended addressing is used (flash > 64kb), allocate the
	//registers RAMPZ and EIND
	if (use_extended_addressing()) {
		if (m_config.rampz)
			m_ioregs[m_config.rampz] = new AVR_IO_Register(true);
		if (m_config.eind)
			m_ioregs[m_config.eind] = new AVR_IO_Register(true);
	}
}

AVR_Core::~AVR_Core()
{
	free(m_sram);
	free(m_flash);
	free(m_flash_tag);

	uint16_t ioreg_size = m_config.ioend - m_config.iostart + 1;
	for (uint16_t i = 0; i < ioreg_size; ++i) {
		if (m_ioregs[i]) delete m_ioregs[i];
	}
	free(m_ioregs);
}

bool AVR_Core::init(AVR_Device& d)
{
	m_device = &d;
	return true;
}

void AVR_Core::reset()
{
	//Jump to the reset interrupt vector
	m_pc = 0;
	//Resets all the general registers to 0x00
	std::memset(m_regs, 0x00, 32);
	//Resets all the I/O register to 0x00.
	//Peripherals are responsible for resetting registers whose reset value is different from 0
	uint16_t ioreg_size = m_config.ioend - m_config.iostart + 1;
	for (int i = 0; i < ioreg_size; ++i) {
		if (m_ioregs[i])
			m_ioregs[i]->set(0);
	}
	//Reset of the SREG register (not handled by the loop above)
	std::memset(m_sreg, 0, 8);
	//Normally this is also done by properly compiled firmware code but just following the HW datasheet here
	write_sp(m_config.ramend);
	//Ensures at least one instruction is executed before interrupts are processed
	m_int_inhib_counter = 1;
}

int AVR_Core::exec_cycle()
{
	//Check if we have a interrupt request and if we can handle it
	int_vect_t irq_vector;
	if ((irq_vector = m_intrctl->cpu_get_irq()) != AVR_INTERRUPT_NONE && m_sreg[SREG_I] && !m_int_inhib_counter) {
		//Acknowledge the vector with the Interrupt Controller
		m_intrctl->cpu_ack_irq();
		//Push the current PC to the stack and jump to the vector table entry
		cpu_push_flash_addr(m_pc >> 1);
		m_pc = irq_vector * m_config.vector_size;
		//Clear the GIE flag if allowed by the core options
		if (m_config.attributes & AVR_CoreConfiguration::ClearGIEOnInt)
			m_sreg[SREG_I] = 0;
	}
	
	//Decrement the instruction counter if used.
	//If it drops to 0, reactivate the interrupts and raise the signal
	if (m_int_inhib_counter) {
		m_int_inhib_counter--;
		//if (!m_int_inhib_counter)
		//	m_int_inhib_signal.raise();
	}
	
	//Executes one instruction and returns the number of clock cycles spent
	int cycles = run_instruction();

	return cycles;
}

void AVR_Core::exec_reti()
{
	//On a RETI, if allowed by the core options, set the GIE flag
	if (m_config.attributes & AVR_CoreConfiguration::ClearGIEOnInt)
		m_sreg[SREG_I] = 1;
	//Inform the Interrupt Controller
	m_intrctl->cpu_reti();
	//Ensures at least one instruction is executed before the next
	//interrupt
	start_interrupt_inhibit(1);
}

void AVR_Core::start_interrupt_inhibit(unsigned int count)
{
	if (m_int_inhib_counter < count)
		m_int_inhib_counter = count;
}


//=======================================================================================
//CPU interface for accessing general purpose working registers (r0 to r31)

uint8_t AVR_Core::cpu_read_gpreg(uint8_t reg)
{
	return m_regs[reg];
}

void AVR_Core::cpu_write_gpreg(uint8_t reg, uint8_t value)
{
	m_regs[reg] = value;
}


//=======================================================================================
//CPU interface for accessing I/O registers.

AVR_IO_Register* AVR_Core::get_ioreg(reg_addr_t addr)
{
	AVR_IO_Register* reg = m_ioregs[addr];
	if (!reg) {
		reg = m_ioregs[addr] = new AVR_IO_Register();
	}
	return reg;
}

uint8_t AVR_Core::cpu_read_ioreg(reg_addr_t io_addr)
{
	uint8_t v;
	if (io_addr == R_SREG) {
		v = read_sreg();
	} else {
		AVR_IO_Register *ioreg = m_ioregs[io_addr];
		if (ioreg) {
			v = ioreg->cpu_read(io_addr);
		} else {
			if (!m_device->test_option(AVR_Device::Option_IgnoreBadCpuIO)) {
				WARNING_LOG(m_device->logger(), "CPU reading an invalid register: %04x", io_addr);
				m_device->crash(CRASH_BAD_CPU_IO, "Invalid CPU register read");
			}
			v = 0;
		}
	}
	return v;
}

void AVR_Core::cpu_write_ioreg(reg_addr_t io_addr, uint8_t value)
{
	if (io_addr == R_SREG) {
		write_sreg(value);
	} else {
		AVR_IO_Register *ioreg = m_ioregs[io_addr];
		if (ioreg) {
			if (ioreg->cpu_write(io_addr, value)) {
				if (!m_device->test_option(AVR_Device::Option_IgnoreBadCpuIO)) {
					WARNING_LOG(m_device->logger(), "CPU writing to a read-only register: %04x", io_addr);
					m_device->crash(CRASH_BAD_CPU_IO, "Register read-only violation");
				}
			}
		} else {
			if (!m_device->test_option(AVR_Device::Option_IgnoreBadCpuIO)) {
				WARNING_LOG(m_device->logger(), "CPU writing to an invalid register: %04x", io_addr);
				m_device->crash(CRASH_BAD_CPU_IO, "Invalid CPU register write");
			}
		}
	}
}


//=======================================================================================
//Peripheral interface for accessing I/O registers.

uint8_t AVR_Core::ioctl_read_ioreg(reg_addr_t addr)
{
	uint8_t v;
	if (addr == R_SREG) {
		v = read_sreg();
	} else {
		AVR_IO_Register *ioreg = m_ioregs[addr];
		if (ioreg) {
			v = ioreg->ioctl_read(addr);
		} else {
			ERROR_LOG(m_device->logger(), "CTL reading an invalid register: %04x", addr);
			m_device->crash(CRASH_BAD_CTL_IO, "Invalid CTL register read");
			v = 0;
		}
	}
	return v;
}

void AVR_Core::ioctl_write_ioreg(regbit_t rb, uint8_t value)
{
	if (rb.addr == R_SREG) {
		uint8_t v = read_sreg();
		v = (v & ~rb.mask) | ((value << rb.bit) & rb.mask);
		write_sreg(v);
	} else {
		AVR_IO_Register *ioreg = m_ioregs[rb.addr];
		if (ioreg) {
			uint8_t v = ioreg->value();
			v = (v & ~rb.mask) | ((value << rb.bit) & rb.mask);
			ioreg->ioctl_write(rb.addr, v);
		} else {
			ERROR_LOG(m_device->logger(), "CTL writing an invalid register: %04x", rb.addr);
			m_device->crash(CRASH_BAD_CTL_IO, "Invalid CTL register write");
		}
	}
}


//=======================================================================================

uint8_t AVR_Core::cpu_read_flash(flash_addr_t pgm_addr)
{
	if (pgm_addr > m_config.flashend) {
		ERROR_LOG(m_device->logger(), "CPU reading an invalid flash address: 0x%04x", pgm_addr);
		m_device->crash(CRASH_FLASH_ADDR_OVERFLOW, "Invalid flash address");
		return 0;
	}

	if (!m_flash_tag[pgm_addr]) {
		WARNING_LOG(m_device->logger(), "CPU reading an unprogrammed flash address: 0x%04x", pgm_addr);
		if (!m_device->test_option(AVR_Device::Option_IgnoreBadCpuLPM)) {
			m_device->crash(CRASH_FLASH_ADDR_OVERFLOW, "Invalid flash address");
		}
	}

	return m_flash[pgm_addr];
}


//=======================================================================================
////CPU helpers for managing the SREG register

uint8_t AVR_Core::read_sreg()
{
	uint8_t v = 0;
	for (int i = 0; i < 8; ++i)
		v |= (m_sreg[i] & 1) << i;
	return v;
}

void AVR_Core::write_sreg(uint8_t value)
{
	for (int i = 0; i < 8; ++i)
		m_sreg[i] = (value >> i) & 1;
}


//=======================================================================================
////CPU helpers for managing the stack

uint16_t AVR_Core::read_sp()
{
	return m_ioregs[R_SPL]->value() | (m_ioregs[R_SPH]->value() << 8);
}

void AVR_Core::write_sp(uint16_t sp)
{
	m_ioregs[R_SPL]->set(sp & 0xFF);
	m_ioregs[R_SPH]->set(sp >> 8);
}

void AVR_Core::cpu_push_flash_addr(flash_addr_t addr)
{
	mem_addr_t sp = read_sp();
	cpu_write_data(sp, addr);
	cpu_write_data(sp - 1, addr >> 8);
	if (use_extended_addressing()) {
		cpu_write_data(sp - 2, addr >> 16);
		write_sp(sp - 3);
	} else {
		write_sp(sp - 2);
	}
}

flash_addr_t AVR_Core::cpu_pop_flash_addr()
{
	flash_addr_t addr;
	mem_addr_t sp = read_sp();
	if (use_extended_addressing()) {
		if ((m_config.ramend - sp) < 3) {
			m_device->crash(CRASH_SP_OVERFLOW, "SP overflow on 24-bits address pop");
			return 0;
		}
		addr = cpu_read_data(sp + 3) | (cpu_read_data(sp + 2) << 8) | (cpu_read_data(sp + 1) << 16);
		write_sp(sp + 3);
	} else {
		if ((m_config.ramend - sp) < 2) {
			m_device->crash(CRASH_SP_OVERFLOW, "SP overflow on 16-bits address pop");
			return 0;
		}
		addr = cpu_read_data(sp + 2) | (cpu_read_data(sp + 1) << 8);
		write_sp(sp + 2);
	}
	return addr;
}


//=======================================================================================

void AVR_Core::dbg_set_debug_probe(AVR_DeviceDebugProbe* debug)
{
	m_debug_probe = debug;
}

/*
 * Block memory mapping from data space to a block of memory
 * The block is defined by the interval [blockstart ; blockend] in data space
 * If the data space block defined by (address/len) intersects with the block,
 * the offsets bufofs, blockofs, blocklen are computed and the function returns true
 */
bool data_space_map(mem_addr_t addr, mem_addr_t len,
				  	mem_addr_t blockstart, mem_addr_t blockend,
					mem_addr_t* bufofs, mem_addr_t* blockofs,
					mem_addr_t* blocklen)
{
	if (addr <= blockend && (addr + len) > blockstart) {
		*bufofs = addr > blockstart ? 0 : (blockstart - addr);
		*blockofs = addr > blockstart ? (addr - blockstart) : 0;
		*blocklen = (addr + len) > blockend ? (blockend - addr + 1) : len;
		return true;
	} else {
		return false;
	}
}
