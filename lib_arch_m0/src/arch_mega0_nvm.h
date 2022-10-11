/*
 * arch_mega0_nvm.h
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

#ifndef __YASIMAVR_MEGA0_NVM_H__
#define __YASIMAVR_MEGA0_NVM_H__

#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "core/sim_memory.h"

//=======================================================================================
/*
 * Implementation of a USERROW controller for Mega0/Mega1 series
 * The purpose of this controller is only to allow reading from the userrow
 * using the data address space.
 */


class DLL_EXPORT AVR_ArchMega0_USERROW : public AVR_Peripheral {

public:

    AVR_ArchMega0_USERROW(reg_addr_t base);

    virtual bool init(AVR_Device& device) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    const reg_addr_t m_reg_base;
    AVR_NonVolatileMemory* m_userrow;

};


//=======================================================================================
/*
 * Implementation of a fuse controller for Mega0/Mega1 series
 * The purpose of this controller is only to allow reading the fuses
 * using the data address space.
 */


class DLL_EXPORT AVR_ArchMega0_Fuses : public AVR_Peripheral {

public:

    AVR_ArchMega0_Fuses(reg_addr_t base);

    virtual bool init(AVR_Device& device) override;

private:

    const reg_addr_t m_reg_base;
    AVR_NonVolatileMemory* m_fuses;

};

//=======================================================================================
/*
 * Implementation of a NVM controller for Mega0/Mega1 series
 * Features:
 *  - supports all commands except WFU(Write FUse)
 *  - the Configuration Change Protection for SPM is not supported (has no effect)
 *  - None of the Write Protection mechanisms (boot lock, boot/app/data section) is supported
 *
 *  CTLREQs supported:
 *   - internally, AVR_CTLREQ_NVM_WRITE is supported to receive NVM write to the page buffer
 */

struct AVR_ArchMega0_NVM_Config {

    reg_addr_t reg_base;

    mem_addr_t flash_page_size;
    //EEPROM page size is assumed to be half the flash page size

    unsigned int buffer_erase_delay;        //Page buffer erase delay in cycles
    //All the delays below are expressed in microseconds
    unsigned int page_write_delay;          //Flash/EEPROM page write operation delay
    unsigned int page_erase_delay;          //Flash/EEPROM page erase operation delay
    unsigned int chip_erase_delay;          //Chip erase delay
    unsigned int eeprom_erase_delay;        //EEPROM erase delay

    int_vect_t iv_eeready;

};

class DLL_EXPORT AVR_ArchMega0_NVM : public AVR_Peripheral {

public:

    AVR_ArchMega0_NVM(const AVR_ArchMega0_NVM_Config& config);
    virtual ~AVR_ArchMega0_NVM();

    virtual bool init(AVR_Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(uint16_t req, ctlreq_data_t* data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

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

    const AVR_ArchMega0_NVM_Config& m_config;
    State m_state;
    uint8_t* m_buffer;
    uint8_t* m_bufset;
    int m_mem_index;
    mem_addr_t m_page;
    Timer* m_timer;

    AVR_InterruptFlag m_ee_intflag;

    AVR_NonVolatileMemory* get_memory(int nvm_index);
    void clear_buffer();
    void write_nvm(const NVM_request_t& nvm_req);
    void execute_command(Command cmd);
    unsigned int execute_page_command(Command cmd);
    void timer_next();

};

#endif //__YASIMAVR_MEGA0_NVM_H__
