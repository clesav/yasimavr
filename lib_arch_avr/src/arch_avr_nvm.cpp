/*
 * arch_avr_nvm.cpp
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

#include "arch_avr_nvm.h"
#include "arch_avr_device.h"
#include "cstring"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

class ArchAVR_NVM::SPM_Timer : public CycleTimer {

public:

    SPM_Timer(ArchAVR_NVM& ctl) : m_ctl(ctl) {}

    virtual cycle_count_t next(cycle_count_t when) override
    {
        m_ctl.spm_timer_next();
        return 0;
    }

private:

    ArchAVR_NVM& m_ctl;

};


class ArchAVR_NVM::EE_Timer : public CycleTimer {

public:

    EE_Timer(ArchAVR_NVM& ctl) : m_ctl(ctl) {}

    virtual cycle_count_t next(cycle_count_t when) override
    {
        m_ctl.ee_timer_next();
        return 0;
    }

private:

    ArchAVR_NVM& m_ctl;

};


ArchAVR_NVM::ArchAVR_NVM(const ArchAVR_NVMConfig& config)
:Peripheral(AVR_IOCTL_NVM)
,m_config(config)
,m_spm_buffer(nullptr)
,m_spm_bufset(nullptr)
,m_spm_page_size(0)
,m_spm_state(State_Idle)
,m_spm_command(0)
,m_spm_timer(nullptr)
,m_halt(false)
,m_ee_state(State_Idle)
,m_ee_prog_mode(0)
,m_section_manager(nullptr)
{
    m_spm_timer = new SPM_Timer(*this);
    m_ee_timer = new EE_Timer(*this);
}


ArchAVR_NVM::~ArchAVR_NVM()
{
    if (m_spm_buffer)
        free(m_spm_buffer);
    if (m_spm_bufset)
        free(m_spm_bufset);

    delete m_spm_timer;
    delete m_ee_timer;
}


bool ArchAVR_NVM::init(Device& device)
{
    bool status = Peripheral::init(device);

    //Allocate the page buffer
    m_spm_page_size = reinterpret_cast<const ArchAVR_CoreConfig&>(device.config().core).flash_page_size;
    m_spm_buffer = (uint8_t*) malloc(m_spm_page_size);
    m_spm_bufset = (uint8_t*) malloc(m_spm_page_size);
    clear_spm_buffer();

    //Allocate the SPM registers
    add_ioreg(regbit_t(m_config.reg_spm_ctrl, m_config.bm_spm_cmd));
    add_ioreg(regbit_t(m_config.reg_spm_ctrl, m_config.bm_spm_enable));
    add_ioreg(regbit_t(m_config.reg_spm_ctrl, m_config.bm_spm_inten));
    add_ioreg(regbit_t(m_config.reg_spm_ctrl, m_config.bm_spm_rww_busy), true);

    //Allocate the EEPROM registers
    add_ioreg(m_config.rbc_ee_addr);
    add_ioreg(m_config.reg_ee_data);
    add_ioreg(m_config.rb_ee_read);
    add_ioreg(m_config.rb_ee_write);
    add_ioreg(m_config.rb_ee_wren);
    add_ioreg(m_config.rb_ee_inten);
    add_ioreg(m_config.rb_ee_mode);

    //Allocate the interrupt vectors
    status &= register_interrupt(m_config.iv_spm_ready, *this);
    status &= register_interrupt(m_config.iv_ee_ready, *this);

    //Obtain the pointer to the flash section manager
    ctlreq_data_t req;
    if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_SECTIONS, &req))
        return false;
    m_section_manager = reinterpret_cast<MemorySectionManager*>(req.data.as_ptr());

    return status;
}


void ArchAVR_NVM::reset()
{
    Peripheral::reset();

    m_spm_state = State_Idle;
    clear_spm_buffer();
    m_halt = false;
    device()->cycle_manager()->cancel(*m_spm_timer);

    //A reset does not stop a EEPROM write, we need to restore the control bits
    if (m_ee_state == State_Write) {
        write_ioreg(m_config.rb_ee_mode, m_ee_prog_mode);
        set_ioreg(m_config.rb_ee_write);
    } else {
        m_ee_state = State_Idle;
        device()->cycle_manager()->cancel(*m_ee_timer);
    }
}


bool ArchAVR_NVM::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    //Read/Write request from the core when executing a LPM or SPM instruction
    if (req == AVR_CTLREQ_NVM_REQUEST) {
        NVM_request_t* nvm_req = reinterpret_cast<NVM_request_t*>(data->data.as_ptr());
        if (nvm_req->kind)
            nvm_req->result = process_NVM_read(*nvm_req);
        else
            nvm_req->result = process_NVM_write(*nvm_req);
        return true;
    }
    return false;
}


void ArchAVR_NVM::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    if (addr == m_config.reg_spm_ctrl) {
        logger().dbg("CPU writing 0x%02x to SPMCR", data.value);
        //If a Flash or eeprom write operation is in progress, the SPM control register is blocked
        if (m_spm_state == State_Idle && m_ee_state != State_Write) {
            //If the enable bit is set, read the command and wait for a SPM instruction (that will arrive
            //in the form of a write request. Start the timer to act as a 4 cycle timeout.
            bool enable = m_config.bm_spm_enable.extract(data.value);
            uint8_t cmd = m_config.bm_spm_cmd.extract(data.value);
            if (enable) {
                m_spm_state = State_Pending;
                m_spm_command = cmd;
                device()->cycle_manager()->delay(*m_spm_timer, 4);
                if (m_spm_command & 0x200)
                    device()->core().set_direct_LPM_enabled(false);

            } else {
                clear_ioreg(m_config.reg_spm_ctrl, m_config.bm_spm_cmd);
            }
        } else {
            write_ioreg(m_config.reg_spm_ctrl, data.old);
        }

        if (m_config.bm_spm_inten.extract(data.value) && m_spm_state == State_Idle)
            raise_interrupt(m_config.iv_spm_ready);
    }

    if (addr == m_config.rb_ee_wren.addr) {
        //On a positive edge on EEPROM write enable, start the 4 cycle window, if the EEPROM is idle
        //and EEPE is zero.
        if (m_config.rb_ee_wren.extract(data.posedge()) && !test_ioreg(m_config.rb_ee_write) && m_ee_state == State_Idle) {
            m_ee_state = State_Pending;
            device()->cycle_manager()->delay(*m_ee_timer, 4);
        } else {
            //In all other cases, EEMPE is not writeable so reinstate the former value.
            write_ioreg(m_config.rb_ee_wren, data.old);
        }
    }

    if (addr == m_config.rb_ee_write.addr) {
        //If we have a strobe on EEPE, start the Write operation, if the write is enabled
        //and no flash write in progress.
        if (m_config.rb_ee_write.extract(data.posedge()) && m_ee_state == State_Pending && m_spm_state == State_Idle) {
            uint8_t prog_mode = read_ioreg(m_config.rb_ee_mode);
            m_ee_state = State_Write;
            start_eeprom_command(prog_mode);
        } else {
            //In all other cases, EEPE is not writeable so reinstate the former value.
            write_ioreg(m_config.rb_ee_write, data.old);
        }
    }

    if (addr == m_config.rb_ee_read.addr) {
        //A 1 write starts a read if the EEPROM if idle
        if (m_config.rb_ee_read.extract(data.posedge()) && m_ee_state <= State_Pending) {
            //Start the EEPROM read operation
            start_eeprom_command(EE_ModeRead);
            //EERE is a strobe bit so clear it
            clear_ioreg(m_config.rb_ee_read);
            //Cancel the Write Enable window
            clear_ioreg(m_config.rb_ee_wren);
        }
    }

    if (addr == m_config.rb_ee_inten.addr) {
        if (m_config.rb_ee_inten.extract(data.value) && !test_ioreg(m_config.rb_ee_write))
            raise_interrupt(m_config.iv_ee_ready);
    }

    if (addr == m_config.rb_ee_mode.addr) {
        //During a EEPROM write, the EEPMx bits are read-only so ensure they are not changed
        if (m_ee_state == State_Write)
            write_ioreg(m_config.rb_ee_mode, data.old);
    }

    if (m_config.rbc_ee_addr.addr_match(addr) || addr == m_config.reg_ee_data) {
        //Changing the address or data register would make a Write in progress fail.
        //Do nothing here, apart from logging an error.
        if (m_ee_state == State_Write)
            logger().err("Writing EEPROM address or data register during a EEPROM Write operation");
    }
}

/*
 * Return a pointer to the NVM area given by index
 */
NonVolatileMemory* ArchAVR_NVM::get_nvm(int nvm_index)
{
    ctlreq_data_t req = { .index = nvm_index };
    if (!device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM, &req))
        return nullptr;
    return reinterpret_cast<NonVolatileMemory*>(req.data.as_ptr());
}


void ArchAVR_NVM::clear_spm_buffer()
{
    memset(m_spm_buffer, 0xFF, m_spm_page_size);
    memset(m_spm_bufset, 0, m_spm_page_size);
}

/*
 * Process read requests : LPM instruction from the core, when direct mode is disabled.
 */
int ArchAVR_NVM::process_NVM_read(NVM_request_t& req)
{
    if (m_spm_state == State_Idle) {
        //Read request, should not happen because the direct LPM mode is disabled.
        //=> bug of the simulator
        logger().err("LPM instruction but no operation enabled.");
        device()->crash(CRASH_INVALID_CONFIG, "LPM instruction but no operation enabled.");
        return -1;
    }
    else if (m_spm_state != State_Pending) {
        //Read request while a command in already progress (can only be a write)
        //We need to check the RWW condition.
        if (m_section_manager->address_access_flags(req.addr) & ArchAVR_Device::Access_RWW) {
            //If trying to read the RWW section, halt the core and return -1 so that the
            //same instruction can be processed again when the core is de-halted.
            ctlreq_data_t d = { .index = 1 };
            device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_HALT, &d);
            m_halt = true;
            return -1;
        } else {
            //Reading the NRWW section, return 0 to carry on with a normal read.
            return 0;
        }
    }

    //Check that the command was expecting a read request. If not, just ignore it and
    //carry on with a normal read.
    if (m_spm_command != SPM_LockBits && m_spm_command != SPM_SigRead)
        return 0;

    //Command to return fuses and lockbits. The address determines which byte is returned.
    if (m_spm_command == SPM_LockBits) {
        switch (req.addr) {
            case 0x0000: { //Fuse low byte
                NonVolatileMemory* fuses = get_nvm(ArchAVR_Core::NVM_Fuses);
                req.data = (*fuses)[0];
            } break;

            case 0x0001: { //Lock bits
                NonVolatileMemory* lockbit = get_nvm(ArchAVR_Core::NVM_Lockbit);
                req.data = (*lockbit)[0];
            } break;

            case 0x0002: { //Fuse extended byte
                NonVolatileMemory* fuses = get_nvm(ArchAVR_Core::NVM_Fuses);
                req.data = (*fuses)[2];
            }break;

            case 0x0003: { //Fuse high byte
                NonVolatileMemory* fuses = get_nvm(ArchAVR_Core::NVM_Fuses);
                req.data = (*fuses)[1];
            } break;

            default:
                logger().err("Invalid Z value for LockBits operation.");
        }

    //Command to read the device signature
    } else { //SPM_SigRead

        switch (req.addr) {
            case 0x0000:
                req.data = m_config.dev_id & 0xFF; break;

            case 0x0002:
                req.data = (m_config.dev_id >> 8) & 0xFF; break;

            case 0x0004:
                req.data = (m_config.dev_id >> 16) & 0xFF; break;

            default:
                logger().err("Invalid Z value for SigRead operation.");
                req.data = 0; break;
        }
    }

    return 1;
}


int ArchAVR_NVM::process_NVM_write(NVM_request_t& req)
{
    req.cycles = 3;

    if (m_spm_state == State_Idle) {
        //Write request with no command set => bug of the firmware
        logger().wng("SPM instruction but no operation enabled.");
        if (device()->test_option(Device::Option_IgnoreBadCpuLPM)) {
            return 0;
        } else {
            device()->crash(CRASH_BAD_CPU_IO, "SPM with disabled NVM controller");
            return -1;
        }
    }
    else if (m_spm_state != State_Pending) {
        //Write request with command already in progress => bug of the firmware
        logger().wng("SPM instruction but NVM controller busy.");
        if (device()->test_option(Device::Option_IgnoreBadCpuLPM)) {
            return 0;
        } else {
            device()->crash(CRASH_BAD_CPU_IO, "SPM with busy NVM controller");
            return -1;
        }
    }

    //Clear the bit 0 of the address, the SPM instruction only uses word-aligned addresses.
    flash_addr_t spm_addr = req.addr & ~1UL;

    //Address range check
    if (spm_addr >= device()->config().core.flashend) {
        logger().err("CPU writing an invalid flash address: 0x%04x", spm_addr);
        device()->crash(CRASH_FLASH_ADDR_OVERFLOW, "Invalid flash address");
        return -1;
    }

    //Check that the operation is allowed wrt. section access control. If not, crash the device.
#ifndef YASIMAVR_NO_ACC_CTRL
    if (!m_section_manager->can_write(spm_addr)) {
        logger().err("CPU writing a locked flash address: 0x%04x", spm_addr);
        if (device()->test_option(Device::Option_IgnoreBadCpuLPM)) {
            return 0;
        } else {
            device()->crash(CRASH_ACCESS_REFUSED, "Flash write refused");
            return -1;
        }
    }
#endif

    if (m_spm_command == SPM_SigRead) return 0;

    cycle_count_t delay = 0;
    flash_addr_t page_offset = spm_addr % m_spm_page_size;

    switch (m_spm_command) {
        case SPM_BufferLoad: {
            logger().dbg("SPM buffer write at flash address: 0x%04x", spm_addr);
            if (!m_spm_bufset[page_offset]) {
                m_spm_buffer[page_offset] = req.data & 0xFF;
                m_spm_buffer[page_offset+1] = (req.data >> 8) & 0xFF;
                m_spm_bufset[page_offset] = 1;
                m_spm_bufset[page_offset+1] = 1;
            }
            req.cycles = 6;
        } break;

        case SPM_PageErase: {
            NonVolatileMemory* flash = get_nvm(ArchAVR_Core::NVM_Flash);
            flash_addr_t spm_page_start = spm_addr - page_offset;
            flash->erase(spm_page_start, m_spm_page_size);
            delay = ((unsigned long long) device()->frequency() * m_config.spm_erase_delay) / 1000000ULL;
            logger().dbg("SPM page erase starting on [0x%04x;0x%04x] (%d cycles)", spm_page_start, spm_page_start + m_spm_page_size - 1, delay);
        } break;

        case SPM_PageWrite: {
            NonVolatileMemory* flash = get_nvm(ArchAVR_Core::NVM_Flash);
            flash_addr_t spm_page_start = spm_addr - page_offset;
            flash->spm_write(m_spm_buffer, m_spm_bufset, spm_page_start, m_spm_page_size);
            clear_spm_buffer();
            delay = ((unsigned long long) device()->frequency() * m_config.spm_write_delay) / 1000000ULL;
            logger().dbg("SPM page write starting on [0x%04x;0x%04x] (%d cycles)", spm_page_start, spm_page_start + m_spm_page_size - 1, delay);
        } break;

        case SPM_LockBits: {
            //TODO:
        } break;
    }

    //Cancel/restart the timer
    device()->cycle_manager()->cancel(*m_spm_timer);
    device()->cycle_manager()->delay(*m_spm_timer, delay);

    if (m_spm_command == SPM_PageErase || m_spm_command == SPM_PageWrite) {
        m_spm_state = State_Write;
        //if writing to a RWW page, we need to control each LPM
        //if writing elsewhere, halt the CPU
        if (m_section_manager->address_access_flags(spm_addr) & ArchAVR_Device::Access_RWW) {
            device()->core().set_direct_LPM_enabled(false);
            set_ioreg(m_config.reg_spm_ctrl, m_config.bm_spm_rww_busy);
        } else {
            logger().dbg("SPM page op out of RWW section : halting the CPU");
            ctlreq_data_t d = { .data = 1 };
            device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_HALT, &d);
            m_halt = true;
        }
    } else {
        m_spm_state = State_Idle;
        clear_ioreg(m_config.reg_spm_ctrl, m_config.bm_spm_cmd);
        clear_ioreg(m_config.reg_spm_ctrl, m_config.bm_spm_enable);
    }

    return 1;
}


void ArchAVR_NVM::spm_timer_next()
{
    if (m_spm_state > State_Pending) {
        logger().dbg("SPM page operation complete");
        if (test_ioreg(m_config.reg_spm_ctrl, m_config.bm_spm_inten))
            raise_interrupt(m_config.iv_spm_ready);
    }

    m_spm_state = State_Idle;

    clear_ioreg(m_config.reg_spm_ctrl, m_config.bm_spm_cmd);
    clear_ioreg(m_config.reg_spm_ctrl, m_config.bm_spm_enable);
    clear_ioreg(m_config.reg_spm_ctrl, m_config.bm_spm_rww_busy);

    device()->core().set_direct_LPM_enabled(true);

    if (m_halt) {
        logger().dbg("Dehalting the CPU");
        //De-halt the core
        ctlreq_data_t d = { .index = 0 };
        device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_HALT, &d);
        m_halt = false;
    }
}


void ArchAVR_NVM::start_eeprom_command(uint8_t command)
{
    //Obtain a pointer to the EEPROM NVM
    NonVolatileMemory* eeprom = get_nvm(ArchAVR_Core::NVM_EEPROM);
    if (!eeprom) {
        logger().err("No access to EEPROM");
        device()->crash(CRASH_INVALID_CONFIG, "EEPROM error");
        return;
    }

    size_t addr = read_ioreg(m_config.rbc_ee_addr);

    cycle_count_t delay = 0;

    switch (command) {
        case EE_ModeRead: {
            m_ee_prog_mode = 0;
            uint8_t data = (*eeprom)[addr];
            write_ioreg(m_config.reg_ee_data, data);
        } break;

        case EE_ModeErase: {
            m_ee_prog_mode = EE_ModeErase;
            eeprom->erase(addr, 1);
            delay = ((unsigned long long) device()->frequency() * m_config.ee_erase_delay) / 1000000ULL;
        } break;

        case EE_ModeWrite: {
            m_ee_prog_mode = EE_ModeWrite;
            uint8_t data = read_ioreg(m_config.reg_ee_data);
            eeprom->spm_write(data, addr);
            delay = ((unsigned long long) device()->frequency() * m_config.ee_write_delay) / 1000000ULL;
        } break;

        case EE_ModeEraseWrite: {
            m_ee_prog_mode = EE_ModeEraseWrite;
            uint8_t data = read_ioreg(m_config.reg_ee_data);
            eeprom->erase(addr, 1);
            eeprom->spm_write(data, addr);
            delay = ((unsigned long long) device()->frequency() * m_config.ee_erase_write_delay) / 1000000ULL;
        } break;
    }

    device()->cycle_manager()->delay(*m_ee_timer, delay);
}


void ArchAVR_NVM::ee_timer_next()
{

    switch (m_ee_state) {
        //Timeout of the Write Enabled
        case State_Pending: {
            clear_ioreg(m_config.rb_ee_wren);
        } break;

        case State_Write: {
            logger().dbg("EEPROM write operation complete");
            //The EEPROM completed a write or erase, clear the bit and raise the interrupt
            clear_ioreg(m_config.rb_ee_write);
            clear_ioreg(m_config.rb_ee_mode);
            if (test_ioreg(m_config.rb_ee_inten))
                raise_interrupt(m_config.iv_ee_ready);
        } break;

        default: break;
    }

    m_ee_state = State_Idle;
}


void ArchAVR_NVM::interrupt_ack_handler(int_vect_t vector)
{
    if (vector == m_config.iv_spm_ready) {
        //If the SPM is idle and the interrupt is still enabled, re-raise it
        if (m_spm_state <= State_Pending && test_ioreg(m_config.reg_spm_ctrl, m_config.bm_spm_inten))
            raise_interrupt(vector);
    }

    else if (vector == m_config.iv_ee_ready) {
        //If the EEPROM is idle and the interrupt is still enabled, re-raise it
        if (m_ee_state <= State_Pending && test_ioreg(m_config.rb_ee_inten))
            raise_interrupt(vector);
    }
}


//=======================================================================================

//Definition for default access control flags
#define SECTION_COUNT     ArchAVR_Device::Section_Count
#define SECTION_APPRWW    ArchAVR_Device::Section_AppRWW
#define SECTION_APPNRWW   ArchAVR_Device::Section_AppNRWW
#define SECTION_BOOT      ArchAVR_Device::Section_Boot
#define ACC_RO            MemorySectionManager::Access_Read
#define ACC_RW            MemorySectionManager::Access_Read | MemorySectionManager::Access_Write
#define ACC_RO_RWW        ACC_RO | ArchAVR_Device::Access_RWW
#define ACC_RW_RWW        ACC_RW | ArchAVR_Device::Access_RWW

#define LOCK_FLAG_SPM     0x01
#define LOCK_FLAG_LPM     0x02


//Default access control definitions for Flash memory sections at reset
const uint8_t DEFAULT_SECTION_FLAGS[SECTION_COUNT][SECTION_COUNT] = {
//To:   AppRWW,      AppNRWW      Boot
      { ACC_RO_RWW,  ACC_RO,      ACC_RO }, //From:AppRWW
      { ACC_RO_RWW,  ACC_RO,      ACC_RO }, //From:AppNRWW
      { ACC_RW_RWW,  ACC_RW,      ACC_RW }, //From:Boot
};


ArchAVR_Fuses::ArchAVR_Fuses(const ArchAVR_FusesConfig& config)
:Peripheral(chr_to_id('F', 'U', 'S', 'E'))
,m_config(config)
,m_fuses(nullptr)
,m_lockbit(nullptr)
,m_sections(nullptr)
{}


bool ArchAVR_Fuses::init(Device& device)
{
    bool status = Peripheral::init(device);

    //Obtain the pointer to the fuse NVM
    ctlreq_data_t req = { .index = ArchAVR_Core::NVM_Fuses };
    if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM, &req))
        return false;
    m_fuses = reinterpret_cast<NonVolatileMemory*>(req.data.as_ptr());

    //Obtain the pointer to the lockbit NVM
    req.index = ArchAVR_Core::NVM_Lockbit;
    if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM, &req))
        return false;
    m_lockbit = reinterpret_cast<NonVolatileMemory*>(req.data.as_ptr());

    //Obtain the pointer to the flash section manager
    if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_SECTIONS, &req))
        return false;
    m_sections = reinterpret_cast<MemorySectionManager*>(req.data.as_ptr());

    return status;
}


void ArchAVR_Fuses::reset()
{
    Peripheral::reset();

    //If it's not a reset on power on, nothing else to do
    ctlreq_data_t req;
    device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET_FLAG, &req);
    if (!(req.data.as_uint() & ArchAVR_Device::Reset_PowerOn))
        return;

    //Load the default access flags
    uint8_t section_flags[SECTION_COUNT][SECTION_COUNT];
    memcpy(section_flags, DEFAULT_SECTION_FLAGS, SECTION_COUNT * SECTION_COUNT);

    //The validity of the bootsize regbit determines if the device has boot loader support
    if (m_config.rb_bootsz.valid()) {
        //Determine the boot section size and set the section limits
        uint8_t bsf = read_fuse(m_config.rb_bootsz);
        int bsi = find_reg_config<ArchAVR_FusesConfig::bootsize_config_t>(m_config.boot_sizes, bsf);
        if (bsi < 0) {
            logger().err("Error in boot size fuse config");
            device()->crash(CRASH_INVALID_CONFIG, "Boot size fuses");
            return;
        }

        flash_addr_t boot_start = m_sections->page_count() - m_config.boot_sizes[bsi].boot_size;
        m_sections->set_section_limits({ m_config.nrww_start, boot_start });

        uint8_t boot_rst = read_fuse(m_config.rb_bootrst);
        uint8_t boot_lockbit = m_config.bm_bootlockbit.extract((*m_lockbit)[0]);

        //if the boot lock bit 0 is cleared, the boot loader cannot write in the boot section
        if (!(boot_lockbit & LOCK_FLAG_SPM))
            section_flags[SECTION_BOOT][SECTION_BOOT] &= ~MemorySectionManager::Access_Write;

        //if the boot lock bit 1 is cleared, the app code cannot read the boot section
        //and if the IVT is in the app section (BOOTRST=1), interrupts are disabled for the boot code
        if (!(boot_lockbit & LOCK_FLAG_LPM)) {
            section_flags[SECTION_APPRWW][SECTION_BOOT] &= ~MemorySectionManager::Access_Read;
            section_flags[SECTION_APPNRWW][SECTION_BOOT] &= ~MemorySectionManager::Access_Read;
            if (boot_rst)
                section_flags[SECTION_BOOT][SECTION_BOOT] |= ArchAVR_Device::Access_IntDisabled;
        }

        uint8_t app_lockbit = m_config.bm_applockbit.extract((*m_lockbit)[0]);

        //if the app lock bit 0 is cleared, the boot loader cannot write in the app sections
        if (!(app_lockbit & LOCK_FLAG_SPM)) {
            section_flags[SECTION_BOOT][SECTION_APPRWW] &= ~MemorySectionManager::Access_Write;
            section_flags[SECTION_BOOT][SECTION_APPNRWW] &= ~MemorySectionManager::Access_Write;
        }

        //if the app lock bit 1 is cleared, the boot loader cannot read the app sections
        //and if the IVT is in the boot section (BOOTRST=0), interrupts are disabled for the app code
        if (!(app_lockbit & LOCK_FLAG_LPM)) {
            section_flags[SECTION_BOOT][SECTION_APPRWW] &= ~MemorySectionManager::Access_Read;
            section_flags[SECTION_BOOT][SECTION_APPNRWW] &= ~MemorySectionManager::Access_Read;
            if (!boot_rst) {
                section_flags[SECTION_APPRWW][SECTION_APPRWW] |= ArchAVR_Device::Access_IntDisabled;
                section_flags[SECTION_APPNRWW][SECTION_APPNRWW] |= ArchAVR_Device::Access_IntDisabled;
            }
        }

    } else {

        //If no bootloader support, make the whole flash an app section
        m_sections->set_section_limits({ m_sections->page_count(), m_sections->page_count() });

    }

    //Set the access control flags in the section manager
    for (unsigned int i = 0; i < SECTION_COUNT; ++i) {
        m_sections->set_fetch_allowed(i,  true);
        for (unsigned int j = 0; j < SECTION_COUNT; ++j)
            m_sections->set_access_flags(i, j, section_flags[i][j]);
    }
}


bool ArchAVR_Fuses::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_FUSE_VALUE) {
        if (data->index == Fuse_BootRst && m_config.rb_bootrst.valid())
            data->data = read_fuse(m_config.rb_bootrst);
        else
            data->data = -1;
        return true;
    }
    return false;
}


uint8_t ArchAVR_Fuses::read_fuse(const regbit_t& rb) const
{
    return rb.extract((*m_fuses)[rb.addr]);
}
