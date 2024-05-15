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
            device()->logger().err("Error in boot size fuse config");
            device()->crash(CRASH_INVALID_CONFIG, "Boot size fuses");
            return;
        }

        flash_addr_t boot_start = m_sections->page_count() - m_config.boot_sizes[bsi].boot_size;
        m_sections->set_section_limits({ m_config.nrww_start, boot_start });

        uint8_t boot_rst = read_fuse(m_config.rb_bootrst);
        uint8_t boot_lockbit = read_fuse(m_config.rb_bootlockbit);

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

        uint8_t app_lockbit = read_fuse(m_config.rb_applockbit);

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
