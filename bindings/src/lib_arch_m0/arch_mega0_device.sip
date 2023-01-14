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

struct AVR_ArchMega0_CoreConfig : public AVR_CoreConfiguration {
%TypeHeaderCode
#include "arch_mega0_device.h"
%End

    mem_addr_t              flashstart_ds;
    mem_addr_t              flashend_ds;
    mem_addr_t              eepromstart_ds;
    mem_addr_t              eepromend_ds;
    mem_addr_t              userrowend;

};

struct AVR_ArchMega0_DeviceConfig : public AVR_DeviceConfiguration {
%TypeHeaderCode
#include "arch_mega0_device.h"
%End
};


class AVR_ArchMega0_Core : public AVR_Core /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_mega0_device.h"
%End

public:

    enum AVR_ArchMega0_NVM /BaseType=IntEnum/ {
        NVM_EEPROM      /PyName=EEPROM/,
        NVM_USERROW     /PyName=USERROW/,
    };

    AVR_ArchMega0_Core(const AVR_ArchMega0_CoreConfig& /KeepReference/);

protected:

    virtual uint8_t cpu_read_data(mem_addr_t);
    virtual void cpu_write_data(mem_addr_t, uint8_t);

};


class AVR_ArchMega0_Device : public AVR_Device /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_mega0_device.h"
%End

public:

    AVR_ArchMega0_Device(const AVR_ArchMega0_DeviceConfig& /KeepReference/);

};
