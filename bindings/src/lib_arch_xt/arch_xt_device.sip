/*
 * arch_nega0_device.sip
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

struct ArchXT_CoreConfig : public CoreConfiguration {
%TypeHeaderCode
#include "arch_xt_device.h"
%End

    mem_addr_t              flashstart_ds;
    mem_addr_t              flashend_ds;
    mem_addr_t              eepromstart_ds;
    mem_addr_t              eepromend_ds;
    mem_addr_t              userrowend;

};


struct ArchXT_DeviceConfig : public DeviceConfiguration  /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_device.h"
%End

    ArchXT_DeviceConfig(CoreConfiguration&);
};


class ArchXT_Core : public Core /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_device.h"
%End

public:

    enum ArchXT_NVM /BaseType=IntEnum/ {
        NVM_EEPROM      /PyName=EEPROM/,
        NVM_USERROW     /PyName=USERROW/,
    };

    ArchXT_Core(const ArchXT_CoreConfig& /KeepReference/);

protected:

    virtual uint8_t cpu_read_data(mem_addr_t);
    virtual void cpu_write_data(mem_addr_t, uint8_t);

};


class ArchXT_Device : public Device /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_device.h"
%End

public:

    enum FlashSection /BaseType=IntEnum/ {
        Section_Boot        /PyName=Boot/,
        Section_AppCode     /PyName=AppCode/,
        Section_AppData     /PyName=AppData/
    };

    ArchXT_Device(const ArchXT_DeviceConfig& /KeepReference/);

};
