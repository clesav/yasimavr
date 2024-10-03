/*
 * arch_avr_nvm.sip
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

struct ArchAVR_NVMConfig {
%TypeHeaderCode
#include "arch_avr_nvm.h"
%End

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

    unsigned int      spm_write_delay;
    unsigned int      spm_erase_delay;
    unsigned int      ee_write_delay;
    unsigned int      ee_erase_delay;
    unsigned int      ee_erase_write_delay;
    int_vect_t        iv_spm_ready;
    int_vect_t        iv_ee_ready;
    uint32_t          dev_id;

};


class ArchAVR_NVM : public Peripheral, public InterruptHandler /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_nvm.h"
%End

public:

    ArchAVR_NVM(const ArchAVR_NVMConfig&);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void interrupt_ack_handler(int_vect_t);

};


//=======================================================================================

%ModuleCode

#include "arch_avr_timer.h"

const ctlreq_id_t CTLREQ_FUSE_VALUE = AVR_CTLREQ_FUSE_VALUE;

%End

const ctlreq_id_t CTLREQ_FUSE_VALUE;


struct ArchAVR_FusesConfig {
%TypeHeaderCode
#include "arch_avr_nvm.h"
%End

    struct bootsize_config_t : base_reg_config_t {
        unsigned long boot_size;
    };

    regbit_t rb_bootsz;
    regbit_t rb_bootrst;
    bitmask_t bm_bootlockbit;
    bitmask_t bm_applockbit;

    flash_addr_t nrww_start;

    std::vector<ArchAVR_FusesConfig::bootsize_config_t> boot_sizes;

};


class ArchAVR_Fuses : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_nvm.h"
%End

public:

    enum Fuses /BaseType=IntEnum/ {
        Fuse_BootRst         /PyName=BootRst/,
    };

    ArchAVR_Fuses(const ArchAVR_FusesConfig&);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);

};
