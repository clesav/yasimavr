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

YASIMAVR_BEGIN_NAMESPACE

class IO_Register;
class Device;
class Firmware;
class InterruptController;
class DeviceDebugProbe;


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

/**
   \brief Breakpoint structure

   Record for the data associated with a software breakpoint.
 */
struct breakpoint_t {
    ///Address in code space of the breakpoint (where the BREAK instruction is inserted)
    flash_addr_t addr;
    ///Instruction replaced by the BREAK (up to 32-bits long)
    uint8_t instr[4];
    ///Length in bytes of the instruction replaced
    uint8_t instr_len;
};


//=======================================================================================

/**
   \brief AVR core generic model

   Base model for a AVR MCU 8-bits core.
   This is an abstract class that the different architecture sub-classes must reimplement.
 */
class AVR_CORE_PUBLIC_API Core {

    friend class Device;
    friend class DeviceDebugProbe;

public:

    /**
       NVM type enum, used for loading a firmware.
       The generic core handles only the flash and fuses.
       The rest (including eeprom) must be handled by
       the architecture implementations.
     */
    enum NVM {
        NVM_Flash,
        NVM_Fuses,
        NVM_CommonCount,
        NVM_ArchDefined = NVM_CommonCount,
        NVM_GetCount = 0xFFFF,
    };

    explicit Core(const CoreConfiguration& config);
    virtual ~Core();

    const CoreConfiguration& config() const;

    bool init(Device& device);

    void reset();

    int exec_cycle();

    IO_Register* get_ioreg(reg_addr_t addr);

    //Peripheral access to the I/O registers
    uint8_t ioctl_read_ioreg(reg_addr_t addr);
    void ioctl_write_ioreg(const regbit_t& rb, uint8_t value);

    void start_interrupt_inhibit(unsigned int count);

    void set_console_register(reg_addr_t addr);

    void set_direct_LPM_enabled(bool enabled);

    //Disable copy semantics
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;

protected:

    ///Reference to the configuration structure, set at construction
    const CoreConfiguration& m_config;
    ///Pointer to the device, set by init()
    Device* m_device;
    ///Array of the 32 general registers
    uint8_t m_regs[32];
    ///Array of the I/O registers
    std::vector<IO_Register*> m_ioregs;
    ///Pointer to the array representing the device RAM memory.
    uint8_t* m_sram;
    ///Non-volatile memory model for the flash.
    NonVolatileMemory m_flash;
    ///Non-volatile memory model for the fuse bits.
    NonVolatileMemory m_fuses;
    ///Program Counter register, expressed in bytes (unlike the actual device PC)
    flash_addr_t m_pc;
    ///Counter to inhibit interrupts for a given number of instructions
    unsigned int m_int_inhib_counter;
    ///Pointer to the generic debug probe
    DeviceDebugProbe* m_debug_probe;
    MemorySectionManager* m_section_manager;

    //CPU access to I/O registers in I/O address space
    uint8_t cpu_read_ioreg(reg_addr_t addr);
    void cpu_write_ioreg(reg_addr_t addr, uint8_t value);

    //CPU access to the general 32 registers
    uint8_t cpu_read_gpreg(uint8_t reg);
    void cpu_write_gpreg(uint8_t reg, uint8_t value);

    /**
       Read memory in data address space. This is a pure virtual function that architectures
       should implement.
       Implementations should ensure that, if the address is invalid,
       the behavior should be consistent with the option IgnoreBadCpuIO.
       \sa cpu_read_ioreg()

       This function is intended for CPU use only.

       \param data_addr Memory address (in 8-bits, data address space) to read

       \return Content at the flash address
     */
    virtual uint8_t cpu_read_data(mem_addr_t data_addr) = 0;

    /**
       Write memory in data address space. This is a pure virtual function that architectures
       should implement.
       Implementations should ensure that, if the address is invalid or read-only,
       the behavior should be consistent with the option IgnoreBadCpuIO.
       \sa cpu_write_ioreg()

       This function is intended for CPU use only.

       \param data_addr Memory address (in 8-bits, data address space) to read
       \param value Value to write
     */
    virtual void cpu_write_data(mem_addr_t data_addr, uint8_t value) = 0;

    int16_t cpu_read_flash(flash_addr_t pgm_addr);

    inline bool use_extended_addressing() const
    {
        return m_config.attributes & CoreConfiguration::ExtendedAddressing;
    }

    //===== Debugging management (used by DeviceDebugProbe) =====

    //Debug probe access to memory data in blocks
    /**
       Read memory in data address space. This is a pure virtual function that architectures
       should implement.

       This function is intended for debug probe use only.

       \param start First memory address (in 8-bits, data address space) to read
       \param buf Buffer where the memory data will be copied into
       \param len Length of the memory block to read
     */
    virtual void dbg_read_data(mem_addr_t start, uint8_t* buf, mem_addr_t len) = 0;

    /**
       Write memory in data address space. This is a pure virtual function that architectures
       should implement.

       This function is intended for debug probe use only.

       \param start First memory address (in 8-bits, data address space) to read
       \param buf Buffer from which the memory data will be copied
       \param len Length of the memory block to write
     */
    virtual void dbg_write_data(mem_addr_t start, const uint8_t* buf, mem_addr_t len) = 0;

    //Breakpoint management
    void dbg_insert_breakpoint(breakpoint_t& bp);
    void dbg_remove_breakpoint(breakpoint_t& bp);

private:

    //Status register variable
    uint8_t m_sreg[8];
    //Direct pointer to the interrupt controller. We don't use the ctlreq framework for performance
    InterruptController* m_intrctl;

    reg_addr_t m_reg_console;
    std::string m_console_buffer;

    //Boolean to indicate if the direct mode for the LPM instruction is enabled
    bool m_direct_LPM;

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

inline const CoreConfiguration& Core::config() const
{
    return m_config;
}

inline void Core::set_console_register(reg_addr_t addr)
{
    m_reg_console = addr;
}


inline void Core::set_direct_LPM_enabled(bool enabled)
{
    m_direct_LPM = enabled;
}


bool data_space_map(mem_addr_t addr, mem_addr_t len,
                    mem_addr_t blockstart, mem_addr_t blockend,
                    mem_addr_t* bufofs, mem_addr_t* blockofs,
                    mem_addr_t* blocklen)
                    AVR_CORE_PUBLIC_API;


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_CORE_H__
