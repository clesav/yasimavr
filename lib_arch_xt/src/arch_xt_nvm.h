/*
 * arch_xt_nvm.h
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

#ifndef __YASIMAVR_XT_NVM_H__
#define __YASIMAVR_XT_NVM_H__

#include "arch_xt_globals.h"
#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "core/sim_memory.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \brief Implementation of a User Row peripheral for XT core series

   The purpose of this class is to allow access to the User Row block from
   the I/O address space.
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_USERROW : public Peripheral {

public:

    explicit ArchXT_USERROW(reg_addr_t base);

    virtual bool init(Device& device) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    const reg_addr_t m_reg_base;
    NonVolatileMemory* m_userrow;

};


//=======================================================================================

/**
   \brief Implementation of a fuse NVM peripheral for XT series

   This controller:
    - allows access to the fuse NVM block from the I/O address space.
    - configures the NVM sections during initialisation from reading the fuse values.
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_Fuses : public Peripheral {

public:

    explicit ArchXT_Fuses(reg_addr_t base);

    virtual bool init(Device& device) override;
    virtual void reset() override;

private:

    const reg_addr_t m_reg_base;
    NonVolatileMemory* m_fuses;
    MemorySectionManager* m_section_manager;

    void configure_flash_sections();

};


//=======================================================================================

/**
   \brief Configuration structure for ArchXT_NVM.
 */
struct ArchXT_NVMConfig {

    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Page size for the flash. The EEPROM page size is assumed to be half of it.
    mem_addr_t flash_page_size;
    /// Page buffer erase delay in cycles
    unsigned int buffer_erase_delay;
    /// Flash/EEPROM page write operation delay in usecs
    unsigned int page_write_delay;
    /// Flash/EEPROM page erase operation delay in usecs
    unsigned int page_erase_delay;
    /// Chip erase operation delay in usecs
    unsigned int chip_erase_delay;
    /// EEPROM erase operation delay in usecs
    unsigned int eeprom_erase_delay;
    /// Interrupt vector index for EEREADY
    int_vect_t iv_eeready;

};

/**
   \brief Implementation of a NVM controller for Mega0/Mega1 series

   Limitations:
    - The Configuration Change Protection for SPM registers has no effect

   CTLREQs supported:
    - AVR_CTLREQ_NVM_REQUEST : Used internally when the CPU writes to a data space address
    mapped to a NVM block. Used to redirect the write to the page buffer.
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_NVM : public Peripheral, public SignalHook {

public:

    explicit ArchXT_NVM(const ArchXT_NVMConfig& config);
    virtual ~ArchXT_NVM();

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    class Timer;
    friend class Timer;

    enum Command {
        Cmd_Idle,
        Cmd_PageWrite,
        Cmd_PageErase,
        Cmd_PageEraseWrite,
        Cmd_BufferErase,
        Cmd_ChipErase,
        Cmd_EEPROMErase,
    };

    enum State {
        State_Idle,
        State_Executing,
        State_Halting,
    };

    const ArchXT_NVMConfig& m_config;
    State m_state;
    uint8_t* m_buffer;
    uint8_t* m_bufset;
    int m_mem_index;
    mem_addr_t m_page;
    Timer* m_timer;

    InterruptFlag m_ee_intflag;

    MemorySectionManager* m_section_manager;
    bool m_pending_bootlock;

    NonVolatileMemory* get_memory(int nvm_index);
    void clear_buffer();
    void write_nvm(NVM_request_t& nvm_req);
    void execute_command(Command cmd);
    unsigned int execute_page_command(Command cmd);
    void timer_next();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_NVM_H__
