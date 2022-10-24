/*
 * sim_core.h
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

#ifndef __YASIMAVR_CORE_H__
#define __YASIMAVR_CORE_H__

#include "sim_types.h"
#include "sim_pin.h"
#include "sim_config.h"
#include "sim_memory.h"
#include <vector>
#include <string>
#include <map>

class AVR_IO_Register;
class AVR_Device;
class AVR_Firmware;
class AVR_InterruptController;
class AVR_DeviceDebugProbe;


//=======================================================================================

//Break opcode, inserted in the program to implement breakpoints
#define AVR_BREAK_OPCODE        0x9598

//Definition of the bit flags for the SREG register
enum {
    SREG_C,// = 0x01,
    SREG_Z,// = 0x02,
    SREG_N,// = 0x04,
    SREG_V,// = 0x08,
    SREG_S,// = 0x10,
    SREG_H,// = 0x20,
    SREG_T,// = 0x40,
    SREG_I,// = 0x80,
};

//Definition of the standard register addresses
enum {
    // 16 bits register pairs
    R_XL    = 0x1a, R_XH,R_YL,R_YH,R_ZL,R_ZH,
    R_X = R_XL,
    R_Y = R_YL,
    R_Z = R_ZL,
    // stack pointer, in IO register space
    R_SPL   = 0x3d, R_SPH,
    // real SREG, in IO register space
    R_SREG  = 0x3f,
};

//Definition of a breakpoint structure
struct breakpoint_t {
    //Address in code space of the breakpoint (where the BREAK instruction is inserted)
    flash_addr_t addr;
    //Instruction replaced by the BREAK (up to 32-bits long)
    uint8_t instr[4];
    //Length in bytes of the instruction replaced
    uint8_t instr_len;
};


//=======================================================================================
/*
 * Implementation of the AVR core CPU
 * This is an abstract class. Concrete sub-classes define the memory map.
*/
class DLL_EXPORT AVR_Core {

    friend class AVR_Device;
    friend class AVR_DeviceDebugProbe;

public:

    enum AVR_NVM {
        NVM_Flash,
        NVM_Fuses,
        NVM_ArchDefined,
    };

    AVR_Core(const AVR_CoreConfiguration& config);
    virtual ~AVR_Core();

    const AVR_CoreConfiguration& config() const;

    bool init(AVR_Device& device);

    void reset();

    int exec_cycle();

    AVR_IO_Register* get_ioreg(reg_addr_t addr);

    //Peripheral access to the I/O registers
    uint8_t ioctl_read_ioreg(reg_addr_t addr);
    void ioctl_write_ioreg(regbit_t rb, uint8_t value);

    void start_interrupt_inhibit(unsigned int count);
    //AVR_Signal& int_inhib_signal();

protected:

    //Reference to the configuration structure, set at construction
    const AVR_CoreConfiguration& m_config;
    //Pointer to the device, set by init()
    AVR_Device* m_device;
    //Value of the 32 general registers
    uint8_t m_regs[32];
    //array of the I/O registers
    AVR_IO_Register** m_ioregs;
    //Pointer to the array representing the device memories.
    uint8_t* m_sram;
    //Non-volatile memory model for the flash.
    AVR_NonVolatileMemory m_flash;
    //Non-volatile memory model for the fuse bits.
    AVR_NonVolatileMemory m_fuses;
    //Top programmed address of the flash in bytes (for PC overflow checks)
    const flash_addr_t m_programend;
    //PC variable, expressed in 8-bits (unlike the actual device PC)
    flash_addr_t m_pc;
    //Counter to inhibit interrupts for a given number of instructions
    unsigned int m_int_inhib_counter;
    //Signal that is raised when the interrupt counter reaches zero
    //AVR_Signal m_int_inhib_signal;
    //Pointer to the generic debug probe
    AVR_DeviceDebugProbe* m_debug_probe;

    //CPU access to I/O registers in I/O address space
    uint8_t cpu_read_ioreg(reg_addr_t addr);
    void cpu_write_ioreg(reg_addr_t addr, uint8_t value);

    //CPU access to the general 32 registers
    uint8_t cpu_read_gpreg(uint8_t reg);
    void cpu_write_gpreg(uint8_t reg, uint8_t value);

    //CPU access to memory data, in data space
    virtual uint8_t cpu_read_data(mem_addr_t data_addr) = 0;
    virtual void cpu_write_data(mem_addr_t data_addr, uint8_t value) = 0;

    //CPU access to the flash with boundary checks
    uint8_t cpu_read_flash(flash_addr_t pgm_addr);

    inline bool use_extended_addressing() const
    {
        return m_config.attributes & AVR_CoreConfiguration::ExtendedAddressing;
    }

    //===== Debugging management (used by AVR_DeviceDebugProbe) =====

    void dbg_set_debug_probe(AVR_DeviceDebugProbe* debug);

    //Debug probe access to memory data in blocks
    virtual void dbg_read_data(mem_addr_t start, uint8_t* buf, mem_addr_t len) = 0;
    virtual void dbg_write_data(mem_addr_t start, uint8_t* buf, mem_addr_t len) = 0;

    //Breakpoint management
    void dbg_insert_breakpoint(breakpoint_t& bp);
    void dbg_remove_breakpoint(breakpoint_t& bp);

private:

    //Status register variable
    uint8_t m_sreg[8];
    //Direct pointer to the interrupt controller. We don't use the ctlreq framework for performance
    AVR_InterruptController* m_intrctl;

    //Helpers for managing the SREG register
    uint8_t read_sreg();
    void write_sreg(uint8_t value);

    //Helpers for managing the stack
    uint16_t read_sp();
    void write_sp(uint16_t sp);
    void cpu_push_flash_addr(flash_addr_t addr);
    flash_addr_t cpu_pop_flash_addr();

    //Main instruction interpreter
    cycle_count_t run_instruction();

    //Called by a RETI instruction
    void exec_reti();

};

inline const AVR_CoreConfiguration& AVR_Core::config() const
{
    return m_config;
}

//inline AVR_Signal& AVR_Core::int_inhib_signal()
//{
//  return m_int_inhib_signal;
//}

bool data_space_map(mem_addr_t addr, mem_addr_t len,
                    mem_addr_t blockstart, mem_addr_t blockend,
                    mem_addr_t* bufofs, mem_addr_t* blockofs,
                    mem_addr_t* blocklen);

#endif //__YASIMAVR_CORE_H__
