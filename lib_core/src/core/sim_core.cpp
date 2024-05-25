/*
 * sim_core.cpp
 *
 *  Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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
#include "sim_debug.h"
#include <cstring>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

/**
   Build a MCU core.

   \param config Configuration settings for the core
 */
Core::Core(const CoreConfiguration& config)
:m_config(config)
,m_device(nullptr)
,m_ioregs(config.ioend - config.iostart + 1, nullptr)
,m_flash(config.flashend + 1)
,m_fuses(config.fusesize)
,m_pc(0)
,m_int_inhib_counter(0)
,m_debug_probe(nullptr)
,m_section_manager(nullptr)
,m_intrctl(nullptr)
,m_direct_LPM(true)
{
    //Allocate the SRAM in RAM
    size_t sram_size = m_config.ramend - m_config.ramstart + 1;
    m_sram = (uint8_t*) malloc(sram_size);
    std::memset(m_sram, 0x00, sram_size);

    //Set the fuses to their default values
    std::vector<unsigned char> f = m_config.fuses;
    m_fuses.program({ f.size(), f.data() });

    //Create the I/O registers managed by the CPU
    m_ioregs[R_SPL] = new IO_Register(true);
    m_ioregs[R_SPH] = new IO_Register(true);

    //If extended addressing is used (flash > 64kb), allocate the
    //registers RAMPZ and EIND
    if (use_extended_addressing()) {
        if (m_config.rampz.valid())
            m_ioregs[m_config.rampz] = new IO_Register(true);
        if (m_config.eind.valid())
            m_ioregs[m_config.eind] = new IO_Register(true);
    }
}

/**
   Destroy a MCU core.
 */
Core::~Core()
{
    free(m_sram);

    for (auto ioreg : m_ioregs) {
        if (ioreg)
            delete ioreg;
    }

    if (m_debug_probe)
        m_debug_probe->detach();
}

/**
   Initialise a MCU core.

   \return the status of initialisation, it's always true.
 */
bool Core::init(Device& d)
{
    m_device = &d;
    if (!m_intrctl) {
        m_device->logger().err("No Interrupt Controller attached");
        return false;
    }
    return true;
}

/**
   Reset the core:
   - the Program Counter is set to 0x0000,
   - all general purpose registers are set to 0x00,
   - all I/O registers are set by default to 0x00 (Peripheral models are responsible for
   resetting registers whose reset values are different),
   - the Stack Pointer is set to RAMEND.
 */
void Core::reset()
{
    //Jump to the reset interrupt vector. 0x0000 is the default Reset Vector address.
    //If the address should be different, it's up to the Device object to rewrite it.
    m_pc = 0;
    //Resets all the general registers to 0x00
    std::memset(m_regs, 0x00, 32);
    //Resets all the I/O register to 0x00.
    //Peripherals are responsible for resetting registers whose reset value is different from 0
    for (auto ioreg : m_ioregs) {
        if (ioreg)
            ioreg->set(0);
    }
    //Reset of the SREG register (not handled by the loop above)
    std::memset(m_sreg, 0, 8);
    //Normally this is also done by properly compiled firmware code but just following the HW datasheet here
    write_sp(m_config.ramend);
    //Ensures at least one instruction is executed before interrupts are processed
    m_int_inhib_counter = 1;
    //Flush the console buffer
    if (m_console_buffer.size())
        m_device->logger().wng("Console output lost by reset");
    m_console_buffer.clear();

    m_direct_LPM = true;
}

/**
   Execute a single instruction cycle with the CPU.

   \return the number of clock cycle consumed by the instruction, or 0
   if something wrong happened.
 */
int Core::exec_cycle()
{
    //Check if we have a interrupt request and if we can handle it
    if (m_intrctl->cpu_has_irq() && !m_int_inhib_counter) {
        InterruptController::IRQ_t irq = m_intrctl->cpu_get_irq();
        //If the GIE flag is set or the vector is non-maskable
        if (m_sreg[SREG_I] || irq.nmi) {
            //Acknowledge the vector with the Interrupt Controller
            m_intrctl->cpu_ack_irq();
            //Push the current PC to the stack and jump to the vector table entry
            cpu_push_flash_addr(m_pc >> 1);
            m_pc = irq.address;
            //Clear the GIE flag if allowed by the core options
            if (m_config.attributes & CoreConfiguration::ClearGIEOnInt)
                m_sreg[SREG_I] = 0;
        }
    }

    //Decrement the instruction counter if used.
    //If it drops to 0, reactivate the interrupts and raise the signal
    if (m_int_inhib_counter)
        m_int_inhib_counter--;

    //Executes one instruction and returns the number of clock cycles spent
    int cycles = run_instruction();

    return cycles;
}

/**
   Execute a RETI instruction with the CPU.
 */
void Core::exec_reti()
{
    //On a RETI, if allowed by the core options, set the GIE flag
    if (m_config.attributes & CoreConfiguration::ClearGIEOnInt)
        m_sreg[SREG_I] = 1;
    //Inform the Interrupt Controller
    m_intrctl->cpu_reti();
    //Ensures at least one instruction is executed before the next
    //interrupt
    start_interrupt_inhibit(1);
}

/**
   Start an interrupt inhibit counter. The counter is decremented for
   each instruction executed after this call. Raised interrupts will be
   held up until this counter has dropped to zero.

   \param count initial inhibit counter value
 */
void Core::start_interrupt_inhibit(unsigned int count)
{
    if (m_int_inhib_counter < count)
        m_int_inhib_counter = count;
}


//=======================================================================================
//CPU interface for accessing general purpose working registers (r0 to r31)

/**
   Read the content of a general purpose register.
   This function is intended for CPU model use only.

   \param reg index of the register to read (0-31)
   \return content of the register
 */
uint8_t Core::cpu_read_gpreg(uint8_t reg)
{
    return m_regs[reg];
}

/**
   Write the content of a general purpose register.
   This function is intended for CPU model use only.

   \param reg index of the register to read (0-31)
   \param value 8-bits value to write in the register
 */
void Core::cpu_write_gpreg(uint8_t reg, uint8_t value)
{
    m_regs[reg] = value;
}


//=======================================================================================
//CPU interface for accessing I/O registers.

/**
   Helper function to get access to IO Registers.
   If a register does not exist, it is allocated.

   \param addr Address of the register to access (in IO address space)

   \return IO_Register object
 */
IO_Register* Core::get_ioreg(reg_addr_t addr)
{
    if (!addr.valid())
        return nullptr;

    IO_Register* reg = m_ioregs[(short) addr];
    if (!reg)
        reg = m_ioregs[(short) addr] = new IO_Register();

    return reg;
}

/**
   Read the content of a I/O register.

   If the register does not exist, by default, the device crashes
   with the code CRASH_BAD_CPU_IO.
   If the option IgnoreBadCpuIO is set, the error is ignored and the value 0x00
   is returned.

   This function is intended for CPU model use only.

   \param addr address of the register to access (in IO address space)

   \return content of the register
 */
uint8_t Core::cpu_read_ioreg(reg_addr_t reg_addr)
{
    if (!reg_addr.valid()) {
        m_device->logger().err("CPU reading an invalid I/O address");
        m_device->crash(CRASH_BAD_CPU_IO, "Invalid CPU register read");
        return 0;
    }

    unsigned short addr = (unsigned short) reg_addr;

    if (addr == R_SREG)
        return read_sreg();

    if (addr >= m_ioregs.size()) {
        m_device->logger().err("CPU reading an off-range I/O address: %04x", addr);
        m_device->crash(CRASH_BAD_CPU_IO, "Invalid CPU register read");
        return 0;
    }

    IO_Register* ioreg = m_ioregs[addr];
    if (ioreg) {
        return ioreg->cpu_read(addr);
    } else {
        if (!m_device->test_option(Device::Option_IgnoreBadCpuIO)) {
            m_device->logger().wng("CPU reading an unregistered I/O address: %04x", addr);
            m_device->crash(CRASH_BAD_CPU_IO, "Invalid CPU register read");
        }
        return 0;
    }
}

/**
   Write the content of a I/O register.

   An error will occurred if the register does not exist, or if the call is modifying
   the read-only part of an existing register.
   If an error occurred, by default, the device crashes with the code CRASH_BAD_CPU_IO.
   If the option IgnoreBadCpuIO is set, the error is ignored and the register is unchanged.

   This function is intended for CPU model use only.

   \param addr address of the register to access (in IO address space)
   \param value value to write
 */
void Core::cpu_write_ioreg(reg_addr_t reg_addr, uint8_t value)
{
    if (!reg_addr.valid()) {
        m_device->logger().err("CPU writing to an invalid I/O address");
        m_device->crash(CRASH_BAD_CPU_IO, "Invalid CPU register write");
        return;
    }

    unsigned short addr = (unsigned short) reg_addr;

    if (addr == R_SREG) {
        write_sreg(value);
        return;
    }

    if (addr >= m_ioregs.size()) {
        m_device->logger().err("CPU writing to an off-range I/O address: %04x", addr);
        m_device->crash(CRASH_BAD_CPU_IO, "Invalid CPU register write");
        return;
    }

    if (addr == m_reg_console) {
        if (value == '\n') {
            char s[20];
            sprintf(s, "[%llu] ", m_device->cycle());
            m_console_buffer.insert(0, s);
            m_device->logger().log(Logger::Level_Output, m_console_buffer.c_str());
            m_console_buffer.clear();
        } else {
            m_console_buffer += (char) value;
        }
        return;
    }

    IO_Register* ioreg = m_ioregs[addr];
    if (ioreg) {
        if (ioreg->cpu_write(addr, value)) {
            if (!m_device->test_option(Device::Option_IgnoreBadCpuIO)) {
                m_device->logger().wng("CPU writing to a read-only register: %04x", addr);
                m_device->crash(CRASH_BAD_CPU_IO, "Register read-only violation");
            }
        }
    } else {
        if (!m_device->test_option(Device::Option_IgnoreBadCpuIO)) {
            m_device->logger().wng("CPU writing to an unregistered I/O address: %04x", addr);
            m_device->crash(CRASH_BAD_CPU_IO, "Invalid CPU register write");
        }
    }
}


//=======================================================================================
//Peripheral interface for accessing I/O registers.

/**
   Read the content of a I/O register.

   If the register does not exist, the device crashes with the code CRASH_BAD_CTL_IO.

   This function is intended for peripheral model use only.

   \param addr address of the register to access (in IO address space)

   \return content of the register
 */
uint8_t Core::ioctl_read_ioreg(const reg_addr_t reg_addr)
{
    if (!reg_addr.valid()) {
        m_device->logger().err("CTL reading an invalid I/O address");
        m_device->crash(CRASH_BAD_CTL_IO, "Invalid CTL register read");
        return 0;
    }

    unsigned short addr = (unsigned short) reg_addr;

    if (addr == R_SREG)
        return read_sreg();

    if (addr >= m_ioregs.size()) {
        m_device->logger().err("CTL reading an off-range I/O address: %04x", addr);
        m_device->crash(CRASH_BAD_CTL_IO, "Invalid CTL register read");
        return 0;
    }

    IO_Register* ioreg = m_ioregs[addr];
    if (ioreg) {
        return ioreg->ioctl_read(addr);
    } else {
        m_device->logger().err("CTL reading an invalid register: %04x", addr);
        m_device->crash(CRASH_BAD_CTL_IO, "Invalid CTL register read");
        return 0;
    }
}

/**
   Write the content of a I/O register.

   If the register does not exist, the device crashes with the code CRASH_BAD_CTL_IO.

   This function is intended for peripheral model use only.

   \param rb regbit of the register/field to access (in IO address space)
   \param value value to write
 */
void Core::ioctl_write_ioreg(const regbit_t& rb, uint8_t value)
{
    if (!rb.valid()) {
        m_device->logger().err("CTL writing to an invalid I/O address");
        m_device->crash(CRASH_BAD_CTL_IO, "Invalid CTL register write");
        return;
    }

    unsigned short addr = (unsigned short) rb.addr;

    if (addr == R_SREG) {
        uint8_t v = read_sreg();
        v = (v & ~rb.mask) | ((value << rb.bit) & rb.mask);
        write_sreg(v);
        return;
    }

    if (addr >= m_ioregs.size()) {
        m_device->logger().err("CTL writing to an off-range I/O address: %04x", addr);
        m_device->crash(CRASH_BAD_CTL_IO, "Invalid CTL register write");
        return;
    }

    IO_Register* ioreg = m_ioregs[addr];
    if (ioreg) {
        uint8_t v = ioreg->value();
        v = (v & ~rb.mask) | ((value << rb.bit) & rb.mask);
        ioreg->ioctl_write(addr, v);
    } else {
        m_device->logger().err("CTL writing to an unregistered I/O address: %04x", addr);
        m_device->crash(CRASH_BAD_CTL_IO, "Invalid CTL register write");
    }
}


//=======================================================================================

/**
   Read the content of the flash non-volatile memory.

   This function is intended for CPU use only, to implement the LPM/ELPM instruction.

   If the address is out of bounds, the device will crash.
   If the address is unprogrammed, by default the device will crash but the error can be ignored
   by setting the option IgnoreBadCpuLPM.

   \param pgm_addr Flash address (in 8-bits, flash address space) to read

   \return value content at the flash address
 */
int16_t Core::cpu_read_flash(flash_addr_t pgm_addr)
{
    //If the LPM direct mode is disabled, obtain the data by a request to the NVM controller.
    if (!m_direct_LPM) {
        NVM_request_t nvm_req = { .kind = 1, .nvm = -1, .addr = pgm_addr, .data = 0, .result = 0 };
        ctlreq_data_t d = { .data = &nvm_req };
        //If the request has been processed, return the value from the request
        if (m_device->ctlreq(AVR_IOCTL_NVM, AVR_CTLREQ_NVM_REQUEST, &d)) {
            if (nvm_req.result > 0)
                return nvm_req.data;
            else if (nvm_req.result < 0)
                return -1;
        }
        //If there is no NVM controller or the request result is 0, revert to the direct mode.
    }

    //Direct mode, first do a range check
    if (pgm_addr > m_config.flashend) {
        m_device->logger().err("CPU reading an invalid flash address: 0x%04x", pgm_addr);
        m_device->crash(CRASH_FLASH_ADDR_OVERFLOW, "Invalid flash address");
        return -1;
    }

    //Access control check
#ifndef YASIMAVR_NO_ACC_CTRL
    if (m_section_manager && !m_section_manager->can_read(pgm_addr)) {
        m_device->logger().err("CPU reading a locked flash address: 0x%04x", pgm_addr);
        m_device->crash(CRASH_ACCESS_REFUSED, "Flash read refused");
        return -1;
    }
#endif

    //Program loading check
    if (!m_flash.programmed(pgm_addr)) {
        m_device->logger().wng("CPU reading an unprogrammed flash address: 0x%04x", pgm_addr);
        if (!m_device->test_option(Device::Option_IgnoreBadCpuLPM)) {
            m_device->crash(CRASH_FLASH_ADDR_OVERFLOW, "Invalid flash address");
        }
    }

    return m_flash[pgm_addr];
}


//=======================================================================================
////CPU helpers for managing the SREG register

uint8_t Core::read_sreg()
{
    uint8_t v = 0;
    for (int i = 0; i < 8; ++i)
        v |= (m_sreg[i] & 1) << i;
    return v;
}

void Core::write_sreg(uint8_t value)
{
    for (int i = 0; i < 8; ++i)
        m_sreg[i] = (value >> i) & 1;
}


//=======================================================================================
////CPU helpers for managing the stack

uint16_t Core::read_sp()
{
    return m_ioregs[R_SPL]->value() | (m_ioregs[R_SPH]->value() << 8);
}

void Core::write_sp(uint16_t sp)
{
    m_ioregs[R_SPL]->set(sp & 0xFF);
    m_ioregs[R_SPH]->set(sp >> 8);
}

void Core::cpu_push_flash_addr(flash_addr_t addr)
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

flash_addr_t Core::cpu_pop_flash_addr()
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

/**
   Block memory mapping from data space to a block of memory
   The block is defined by the interval [blockstart ; blockend] in data space
   If the data space block defined by (address/len) intersects with the block,
   the offsets bufofs, blockofs, blocklen are computed and the function returns true
 */
bool YASIMAVR_QUALIFIED_NAME(data_space_map)(mem_addr_t addr, mem_addr_t len,
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
