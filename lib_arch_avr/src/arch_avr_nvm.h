/*
 * arch_avr_nvm.h
 *
 *  Copyright 2024 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_AVR_NVM_H__
#define __YASIMAVR_AVR_NVM_H__

#include "core/sim_types.h"
#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "core/sim_memory.h"
#include "arch_avr_globals.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \brief Configuration structure for ArchAVR_NVM.
 */
struct ArchAVR_NVMConfig {

    reg_addr_t        reg_spm_ctrl;
    bitmask_t         bm_spm_cmd;
    bitmask_t         bm_spm_enable;
    bitmask_t         bm_spm_inten;
    bitmask_t         bm_spm_rww_busy;

    regbit_compound_t rbc_ee_addr;
    reg_addr_t        reg_ee_data;
    regbit_t          rb_ee_read;
    regbit_t          rb_ee_write;
    regbit_t          rb_ee_wren;
    regbit_t          rb_ee_inten;
    regbit_t          rb_ee_mode;

    /// Flash/EEPROM page write operation delay in usecs
    unsigned int      spm_write_delay;
    /// Flash/EEPROM page erase operation delay in usecs
    unsigned int      spm_erase_delay;
    /// EEPROM Write delay in usecs
    unsigned int      ee_write_delay;
    /// EEPROM Erase delay in usecs
    unsigned int      ee_erase_delay;
    /// EEPROM Erase/Write delay in usecs
    unsigned int      ee_erase_write_delay;
    /// Interrupt vector for SPM
    int_vect_t        iv_spm_ready;
    /// Interrupt vector index for EEREADY
    int_vect_t        iv_ee_ready;
    /// Device ID
    uint32_t          dev_id;

};


/**
   \brief Implementation of a NVM controller for AVR series

   Limitations:
    - The Configuration Change Protection for SPM registers has no effect

   CTLREQs supported:
    - AVR_CTLREQ_NVM_REQUEST : Used internally when the CPU writes to a data space address
    mapped to a NVM block. Used to redirect the write to the page buffer.
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_NVM : public Peripheral, public InterruptHandler {

public:

    explicit ArchAVR_NVM(const ArchAVR_NVMConfig& config);
    virtual ~ArchAVR_NVM();

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void interrupt_ack_handler(int_vect_t vector) override;

private:

    class SPM_Timer;
    friend class SPM_Timer;

    class EE_Timer;
    friend class EE_Timer;

    enum State {
        State_Idle = 0,
        State_Pending,
        State_Read,
        State_Write,
    };

    enum SPM_Command {
        SPM_BufferLoad  = 0x00,
        SPM_PageErase   = 0x01,
        SPM_PageWrite   = 0x02,
        SPM_LockBits    = 0x04,
        SPM_RWWEnable   = 0x08,
        SPM_SigRead     = 0x10,
    };

    enum EEPROM_Mode {
        EE_ModeEraseWrite = 0,
        EE_ModeErase,
        EE_ModeWrite,
        EE_ModeRead
    };

    const ArchAVR_NVMConfig& m_config;

    uint8_t* m_spm_buffer;
    uint8_t* m_spm_bufset;
    flash_addr_t m_spm_page_size;
    State m_spm_state;
    int m_spm_command;
    SPM_Timer* m_spm_timer;
    bool m_halt;

    State m_ee_state;
    uint8_t m_ee_prog_mode;
    EE_Timer* m_ee_timer;

    MemorySectionManager* m_section_manager;

    NonVolatileMemory* get_nvm(int nvm_index);
    void clear_spm_buffer();
    int process_NVM_read(NVM_request_t& req);
    int process_NVM_write(NVM_request_t& req);
    void spm_timer_next();
    void start_eeprom_command(uint8_t command);
    void ee_timer_next();

};


//=======================================================================================

/**
   \file
   \name Controller requests definition for ArchAVR_Fuses
   @{
 */

/**
   Request to obtain the value of a fuse.

   index should be one of ArchAVR_Fuses::Fuses enum values
 */
#define AVR_CTLREQ_FUSE_VALUE          1

/// @}


/**
   \brief Configuration structure for ArchAVR_Fuses.
 */
struct ArchAVR_FusesConfig {

    struct bootsize_config_t : base_reg_config_t {
        unsigned long boot_size;
    };

    /// Regbit for the boot size fuse bits
    regbit_t                         rb_bootsz;
    /// Regbit for the boot reset fuse bit
    regbit_t                         rb_bootrst;
    /// Regbit for the boot part of the lockbits
    bitmask_t                        bm_bootlockbit;
    /// Regbit for the application part of the lockbits
    bitmask_t                        bm_applockbit;

    /// Start of NRWW (No Read-While-Write) section, in number of section pages
    flash_addr_t                     nrww_start;
    /// Boot_sizes mapping, in number of section pages
    std::vector<bootsize_config_t>   boot_sizes;

};

/**
   \brief Implementation of a fuse NVM peripheral for AVR series

   The purpose of this controller is to use the values of the fuses to configure the section
   access control flags on a device reset.
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_Fuses : public Peripheral {

public:

    enum Fuses {
        Fuse_BootRst,
    };

    explicit ArchAVR_Fuses(const ArchAVR_FusesConfig& config);

    virtual bool init(Device& device) override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void reset() override;

private:

    const ArchAVR_FusesConfig& m_config;

    NonVolatileMemory* m_fuses;
    NonVolatileMemory* m_lockbit;
    MemorySectionManager* m_sections;

    void configure_sections();
    uint8_t read_fuse(const regbit_t& rb) const;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_NVM_H__
