/*
 * arch_xt_nvm.sip
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

class ArchXT_USERROW : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_nvm.h"
%End

public:

    ArchXT_USERROW(reg_addr_t);

    virtual bool init(Device&);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
};


class ArchXT_Fuses : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_nvm.h"
%End

public:

    ArchXT_Fuses(reg_addr_t);

    virtual bool init(Device&);
    virtual void reset();

};


struct ArchXT_NVMConfig {
%TypeHeaderCode
#include "arch_xt_nvm.h"
%End

    reg_addr_t reg_base;

    mem_addr_t flash_page_size;

    unsigned int buffer_erase_delay;

    unsigned int page_write_delay;
    unsigned int page_erase_delay;

    unsigned int chip_erase_delay;
    unsigned int eeprom_erase_delay;

    int_vect_t iv_eeready;

};


class ArchXT_NVM : public Peripheral, public SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_nvm.h"
%End

public:

    ArchXT_NVM(const ArchXT_NVMConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void raised(const signal_data_t&, int);

};
