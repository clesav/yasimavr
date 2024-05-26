/*
 * arch_xt_nvm.cpp
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

#include "arch_xt_nvm.h"
#include "arch_xt_device.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "core/sim_device.h"
#include "cstring"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

ArchXT_USERROW::ArchXT_USERROW(reg_addr_t base)
:Peripheral(chr_to_id('U', 'R', 'O', 'W'))
,m_reg_base(base)
,m_userrow(nullptr)
{}


bool ArchXT_USERROW::init(Device& device)
{
    bool status = Peripheral::init(device);

    //Obtain the pointer to the userrow block in RAM
    ctlreq_data_t req = { .index = ArchXT_Core::NVM_USERROW };
    if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM, &req))
        return false;
    m_userrow = reinterpret_cast<NonVolatileMemory*>(req.data.as_ptr());

    //Allocate a register for each byte of the userrow block
    //And initialise it with the value contained in the userrow block
    for (size_t i = 0; i < sizeof(USERROW_t); ++i) {
        add_ioreg(m_reg_base + i);
        write_ioreg(m_reg_base + i, (*m_userrow)[i]);
    }

    return status;
}


void ArchXT_USERROW::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    //Send a NVM write request with the new data value
    NVM_request_t nvm_req = {
        .kind = 0,
        .nvm = ArchXT_Core::NVM_USERROW,
        .addr = (mem_addr_t) addr - m_reg_base, //translate the address into userrow space
        .data = data.value,
        .result = 0,
    };
    ctlreq_data_t d = { .data = &nvm_req };
    device()->ctlreq(AVR_IOCTL_NVM, AVR_CTLREQ_NVM_REQUEST, &d);
    //The write operation is only effective by a command to the NVM controller
    //Meanwhile, reading the register after a write actually returns the NVM block value
    //not yet overwritten. Here this value is in data.old and must be restored into the register.
    write_ioreg(addr, data.old);
}


//=======================================================================================

//Definition for default access control flags
#define SECTION_COUNT     ArchXT_Device::Section_Count
#define ACC_RO            MemorySectionManager::Access_Read
#define ACC_RW            MemorySectionManager::Access_Read | MemorySectionManager::Access_Write


//Default access control definitions for Flash memory sections at reset:
// - All sections are readable from anywhere
// - Boot can write to AppCode and AppData
// - AppCode can write to AppData only
// - AppData cannot write
const uint8_t SECTION_ACCESS_FLAGS[SECTION_COUNT][SECTION_COUNT] = {
//To:   Boot,       AppCode     AppData
      { ACC_RO,     ACC_RW,     ACC_RW }, //From:Boot
      { ACC_RO,     ACC_RO,     ACC_RW }, //From:AppCode
      { ACC_RO,     ACC_RO,     ACC_RO }, //From:AppData
};


/**
   Constructor of a Fuse controller.
   \param base base address in the data space for the fuse bytes
 */
ArchXT_Fuses::ArchXT_Fuses(reg_addr_t base)
:Peripheral(chr_to_id('F', 'U', 'S', 'E'))
,m_reg_base(base)
,m_fuses(nullptr)
,m_section_manager(nullptr)
{}


bool ArchXT_Fuses::init(Device& device)
{
    bool status = Peripheral::init(device);

    //Obtain the pointer to the fuse block in RAM
    ctlreq_data_t req = { .index = ArchXT_Core::NVM_Fuses };
    if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM, &req))
        return false;
    m_fuses = reinterpret_cast<NonVolatileMemory*>(req.data.as_ptr());

    //Allocate a register in read-only access for each fuse
    for (unsigned int i = 0; i < sizeof(FUSE_t); ++i)
        add_ioreg(m_reg_base + i, 0xFF, true);

    //Obtain the pointer to the flash section manager
    if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_SECTIONS, &req))
        return false;
    m_section_manager = reinterpret_cast<MemorySectionManager*>(req.data.as_ptr());

    return status;
}


void ArchXT_Fuses::reset()
{
    for (unsigned int i = 0; i < sizeof(FUSE_t); ++i)
        write_ioreg(m_reg_base + i, (*m_fuses)[i]);

    configure_flash_sections();
}


/*
 * This function configures the flash sections according to the value of the fuses BOOTEND and APPEND.
 * The flash has 3 sections Boot, AppCode, AppData.
 */
void ArchXT_Fuses::configure_flash_sections()
{
    //Read the BOOTEND and APPEND fuse values as page numbers (of 256 bytes)
    flash_addr_t bootend = (*m_fuses)[offsetof(FUSE_t, BOOTEND)];
    flash_addr_t append = (*m_fuses)[offsetof(FUSE_t, APPEND)];
    //Flash end as a page number
    flash_addr_t flashend = (device()->core().config().flashend + 1) / ArchXT_Device::SECTION_PAGE_SIZE;

    //Log a warning if the fuse values are off limits.
    if (bootend > flashend || append > flashend)
        logger().wng("Invalid fuses values: BOOTEND=%d, APPEND=%d", bootend, append);

    //Go through the various combinations of bootend/append values to find the section boundaries
    flash_addr_t limit1, limit2;
    if (!bootend || bootend > flashend) {
        //If BOOTEND is zero, the entire flash is boot
        limit1 = limit2 = flashend;
    } else {
        limit1 = bootend;
        if (!append || append > flashend)
            limit2 = flashend; //makes the AppData section empty
        else if (append < bootend)
            limit2 = bootend; //makes the AppCode section empty
        else
            limit2 = append;
    }

    m_section_manager->set_section_limits({ limit1, limit2 });

    //Set the default access control flags
    for (unsigned int i = 0; i < SECTION_COUNT; ++i) {
        for (unsigned int j = 0; j < SECTION_COUNT; ++j)
            m_section_manager->set_access_flags(i, j, SECTION_ACCESS_FLAGS[i][j]);

        //Set all sections as executable by default
        m_section_manager->set_fetch_allowed(i, true);
    }
}


//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base + offsetof(NVMCTRL_t, reg))

#define REG_OFS(reg) \
    offsetof(NVMCTRL_t, reg)


#define NVM_INDEX_NONE      -1
#define NVM_INDEX_INVALID   -2


class ArchXT_NVM::Timer : public CycleTimer {

public:

    Timer(ArchXT_NVM& ctl) : m_ctl(ctl) {}

    virtual cycle_count_t next(cycle_count_t when) override {
        m_ctl.timer_next();
        return 0;
    }

private:

    ArchXT_NVM& m_ctl;

};


ArchXT_NVM::ArchXT_NVM(const ArchXT_NVMConfig& config)
:Peripheral(AVR_IOCTL_NVM)
,m_config(config)
,m_state(State_Idle)
,m_buffer(nullptr)
,m_bufset(nullptr)
,m_mem_index(NVM_INDEX_NONE)
,m_page(0)
,m_ee_intflag(false)
,m_section_manager(nullptr)
,m_pending_bootlock(false)
{
    m_timer = new Timer(*this);
}


ArchXT_NVM::~ArchXT_NVM()
{
    delete m_timer;

    if (m_buffer)
        free(m_buffer);
    if (m_bufset)
        free(m_bufset);
}


bool ArchXT_NVM::init(Device& device)
{
    bool status = Peripheral::init(device);

    //Allocate the page buffer
    m_buffer = (uint8_t*) malloc(m_config.flash_page_size);
    m_bufset = (uint8_t*) malloc(m_config.flash_page_size);

    //Allocate the registers
    add_ioreg(REG_ADDR(CTRLA), NVMCTRL_CMD_gm);
    add_ioreg(REG_ADDR(CTRLB), NVMCTRL_BOOTLOCK_bm | NVMCTRL_APCWP_bm);
    add_ioreg(REG_ADDR(STATUS), NVMCTRL_WRERROR_bm | NVMCTRL_EEBUSY_bm | NVMCTRL_FBUSY_bm, true);
    add_ioreg(REG_ADDR(INTCTRL), NVMCTRL_EEREADY_bm);
    add_ioreg(REG_ADDR(INTFLAGS), NVMCTRL_EEREADY_bm);
    //DATA and ADDR not implemented

    status &= m_ee_intflag.init(device,
                                DEF_REGBIT_B(INTCTRL, NVMCTRL_EEREADY),
                                DEF_REGBIT_B(INTFLAGS, NVMCTRL_EEREADY),
                                m_config.iv_eeready);

    //Obtain the pointer to the flash section manager
    ctlreq_data_t req;
    if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_SECTIONS, &req))
        return false;
    m_section_manager = reinterpret_cast<MemorySectionManager*>(req.data.as_ptr());

    return status;
}


void ArchXT_NVM::reset()
{
    //Erase the page buffer
    clear_buffer();
    //Set the EEPROM Ready flag
    m_ee_intflag.set_flag();
    //Internals
    m_state = State_Idle;
    m_pending_bootlock = false;
}


bool ArchXT_NVM::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    //NVM request from the core when writing to a data space
    //location mapped to one of the NVM blocks
    if (req == AVR_CTLREQ_NVM_REQUEST) {
        NVM_request_t* nvm_req = reinterpret_cast<NVM_request_t*>(data->data.as_ptr());

        //Only process write requests
        if (!nvm_req->kind) {

        //Check that the operation is allowed wrt. section access control. If not, crash the device.
    #ifndef YASIMAVR_NO_ACC_CTRL
            if (!m_section_manager->can_write(nvm_req->addr)) {
                device()->logger().err("CPU writing a locked flash address: 0x%04x", nvm_req->addr);
                device()->crash(CRASH_ACCESS_REFUSED, "Flash write refused");
                nvm_req->result = -1;
                return true;
            }
    #endif

            write_nvm(*nvm_req);
        }

        return true;
    }
    return false;
}


void ArchXT_NVM::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        Command cmd = (Command) EXTRACT_F(data.value, NVMCTRL_CMD);
        execute_command(cmd);
        WRITE_IOREG_F(CTRLA, NVMCTRL_CMD, 0x00);
    }

    else if (reg_ofs == REG_OFS(CTRLB)) {
        //If APCWP is written to 1
        if (EXTRACT_B(data.posedge(), NVMCTRL_APCWP)) {
            //Change all the access control flags where APPCODE is the destination section to clear the WRITE flag
            for (unsigned int i = 0; i < SECTION_COUNT; ++i) {
                uint8_t f = m_section_manager->access_flags(i, ArchXT_Device::Section_AppCode);
                m_section_manager->set_access_flags(i, ArchXT_Device::Section_AppCode, f & ~MemorySectionManager::Access_Write);
            }
        }

        //If BOOTLOCK is written to 1
        if (EXTRACT_B(data.posedge(), NVMCTRL_BOOTLOCK)) {
            //Writing BOOTLOCK is only allowed from the boot section
            if (m_section_manager->current_section() == ArchXT_Device::Section_Boot)
                //Defer the boot lock until exiting the boot section
                m_pending_bootlock = true;
            else
                CLEAR_IOREG(CTRLB, NVMCTRL_BOOTLOCK);
        }

        //Prevent attempts to clear the APCWP or BOOTLOCK bits
        if (EXTRACT_B(data.negedge(), NVMCTRL_APCWP))
            SET_IOREG(CTRLB, NVMCTRL_APCWP);
        if (EXTRACT_B(data.negedge(), NVMCTRL_BOOTLOCK))
            SET_IOREG(CTRLB, NVMCTRL_BOOTLOCK);
    }

    else if (reg_ofs == REG_OFS(INTCTRL)) {
        m_ee_intflag.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(INTFLAGS)) {
        m_ee_intflag.clear_flag(data.value);
    }
}


NonVolatileMemory* ArchXT_NVM::get_memory(int nvm_index)
{
    ctlreq_data_t req = { .index = nvm_index };
    if (!device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM, &req))
        return nullptr;
    return reinterpret_cast<NonVolatileMemory*>(req.data.as_ptr());
}


void ArchXT_NVM::clear_buffer()
{
    memset(m_buffer, 0xFF, m_config.flash_page_size);
    memset(m_bufset, 0, m_config.flash_page_size);
    m_mem_index = NVM_INDEX_NONE;
}


void ArchXT_NVM::write_nvm(NVM_request_t& nvm_req)
{
    nvm_req.result = -1;

    if (m_mem_index == NVM_INDEX_INVALID) return;

    //Determine the page size, depending on which NVM block is
    //addressed
    mem_addr_t page_size;
    int block;
    if (nvm_req.nvm == ArchXT_Core::NVM_Flash) {
        block = ArchXT_Core::NVM_Flash;
        page_size = m_config.flash_page_size;
    }
    else if (nvm_req.nvm == ArchXT_Core::NVM_EEPROM) {
        block = ArchXT_Core::NVM_EEPROM;
        page_size = m_config.flash_page_size >> 1;
    }
    else if (nvm_req.nvm == ArchXT_Core::NVM_USERROW) {
        block = ArchXT_Core::NVM_USERROW;
        page_size = m_config.flash_page_size >> 1;
    }
    else {
        m_mem_index = NVM_INDEX_INVALID;
        return;
    }

    //Stores the addressed block and check the consistency with
    //any previous NVM write. They should be to the same block.
    //If not, the operation is invalidated.
    if (m_mem_index == NVM_INDEX_NONE)
        m_mem_index = block;
    else if (block != m_mem_index) {
        m_mem_index = NVM_INDEX_INVALID;
        return;
    }

    //Write to the page buffer
    mem_addr_t page_offset = nvm_req.addr % page_size;
    m_buffer[page_offset] &= nvm_req.data;
    m_bufset[page_offset] = 1;

    //Storing the page number
    m_page = nvm_req.addr / page_size;

    nvm_req.result = 1;

    logger().dbg("Buffer write addr=%04x, index=%d, page=%d, value=%02x",
                 nvm_req.addr, m_mem_index, m_page, nvm_req.data);
}


void ArchXT_NVM::execute_command(Command cmd)
{
    cycle_count_t delay = 0;
    unsigned int delay_usecs = 0;

    CLEAR_IOREG(CTRLA, NVMCTRL_WRERROR);

    if (cmd == Cmd_Idle) {
        //Nothing to do
        return;
    }
    else if (cmd == Cmd_BufferErase) {
        //Clear the buffer and set the CPU halt (the delay is expressed in cycles)
        clear_buffer();
        m_state = State_Halting;
        delay = m_config.buffer_erase_delay;
    }

    else if (cmd == Cmd_ChipErase) {
        //Erase the flash
        NonVolatileMemory* flash = get_memory(ArchXT_Core::NVM_Flash);
        if (flash)
            flash->erase();
        //Erase the eeprom
        NonVolatileMemory* eeprom = get_memory(ArchXT_Core::NVM_EEPROM);
        if (eeprom)
            eeprom->erase();
        //Set the halt state and delay
        m_state = State_Halting;
        delay_usecs = m_config.chip_erase_delay;
    }

    else if (cmd == Cmd_EEPROMErase) {
        //Erase the eeprom
        NonVolatileMemory* eeprom = get_memory(ArchXT_Core::NVM_EEPROM);
        if (eeprom)
            eeprom->erase();
        //Set the halt state and delay
        m_state = State_Halting;
        delay_usecs = m_config.eeprom_erase_delay;
    }

    //The remaining commands require a valid block & page selection
    else if (m_mem_index >= 0) {
        delay_usecs = execute_page_command(cmd);
    }
    else {
        SET_IOREG(CTRLA, NVMCTRL_WRERROR);
    }

    if (delay_usecs)
        delay = (device()->frequency() * delay_usecs) / 1000000L;

    //Halt the core if required by the command and set the timer
    //to simulate the operation completion delay
    if (delay) {
        if (m_state == State_Halting) {
            ctlreq_data_t d = { .index = 1 };
            device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_HALT, &d);
        }

        device()->cycle_manager()->delay(*m_timer, delay);
    }
}


unsigned int ArchXT_NVM::execute_page_command(Command cmd)
{
    unsigned int delay_usecs = 0;

    //Boolean indicating if it's an operation to the flash (true)
    //or to the eeprom (false)
    bool is_flash_op = (m_mem_index == ArchXT_Core::NVM_Flash);

    //Obtain the pointer to the NVM object
    NonVolatileMemory* nvm = get_memory(m_mem_index);
    if (!nvm) {
        device()->crash(CRASH_INVALID_CONFIG, "Bad memory block");
        return 0;
    }

    //Get the page size
    size_t page_size = m_config.flash_page_size;
    if (!is_flash_op)
        page_size /= 2;

    //Erase the page if required by the command
    //If it's to the flash, it's the whole page, otherwise
    //it's to the eeprom with a byte granularity
    if (cmd == Cmd_PageErase || cmd == Cmd_PageEraseWrite) {
        if (is_flash_op) {
            nvm->erase(page_size * m_page, page_size);
            logger().dbg("Erased flash page %d", m_page);
        } else {
            logger().dbg("Erased eeprom/userrow page %d", m_page);
            nvm->erase(m_bufset, page_size * m_page, page_size);
        }

        delay_usecs += m_config.page_erase_delay;
    }

    //Write the page if required by the command
    //If it's to the flash, it's the whole page, otherwise
    //it's to the eeprom/userrow with a byte granularity
    if (cmd == Cmd_PageWrite || cmd == Cmd_PageEraseWrite) {
        if (is_flash_op) {
            nvm->spm_write(m_buffer, nullptr, page_size * m_page, page_size);
            logger().dbg("Written flash page %d", m_page);
        } else {
            nvm->spm_write(m_buffer, m_bufset, page_size * m_page, page_size);
            logger().dbg("Written eeprom/userrow page %d", m_page);
        }

        delay_usecs += m_config.page_write_delay;
    }

    //Clears the page buffer
    clear_buffer();

    //Update the state and the status flags
    if (is_flash_op) {
        m_state = State_Halting;
        SET_IOREG(STATUS, NVMCTRL_FBUSY);
    } else {
        m_state = State_Executing;
        SET_IOREG(STATUS, NVMCTRL_EEBUSY);
        m_ee_intflag.clear_flag();
    }

    return delay_usecs;
}


void ArchXT_NVM::timer_next()
{
    //Update the status flags
    CLEAR_IOREG(STATUS, NVMCTRL_FBUSY);
    CLEAR_IOREG(STATUS, NVMCTRL_EEBUSY);
    m_ee_intflag.set_flag();
    //If the CPU was halted, allow it to resume
    if (m_state == State_Halting) {
        ctlreq_data_t d = { .index = 0 };
        device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_HALT, &d);
    }
    //Update the state
    m_state = State_Idle;
}


void ArchXT_NVM::raised(const signal_data_t& sigdata, int)
{
    //If there is a pending bootlock and the CPU is leaving the boot section
    if (m_pending_bootlock &&
        sigdata.sigid == MemorySectionManager::Signal_Leave &&
        sigdata.data == ArchXT_Device::Section_Boot) {

        //Clear all access rights to the boot section
        m_section_manager->set_access_flags(ArchXT_Device::Section_AppCode, ArchXT_Device::Section_Boot, 0x00);
        m_section_manager->set_access_flags(ArchXT_Device::Section_AppData, ArchXT_Device::Section_Boot, 0x00);
        //Set the boot section as non-executable
        m_section_manager->set_fetch_allowed(ArchXT_Device::Section_Boot, false);

        m_pending_bootlock = false;
    }

}
