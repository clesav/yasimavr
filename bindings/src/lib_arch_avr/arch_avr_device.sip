/*
 * arch_avr_device.sip
 *
 *  Copyright 2021-2024 Clement Savergne <csavergne@yahoo.com>

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

struct ArchAVR_CoreConfig : public CoreConfiguration {
%TypeHeaderCode
#include "arch_avr_device.h"
%End

    flash_addr_t flash_page_size;

    flash_addr_t flash_page_count() const;

};

struct ArchAVR_DeviceConfig : public DeviceConfiguration /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_device.h"
%End

    ArchAVR_DeviceConfig(ArchAVR_CoreConfig&);

};


class ArchAVR_Core : public Core /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_device.h"
%End

public:

    enum ArchAVR_NVM /BaseType=IntEnum/ {
        NVM_EEPROM          /PyName=EEPROM/,
        NVM_Lockbit         /PyName=Lockbit/,
    };

    ArchAVR_Core(const ArchAVR_CoreConfig& /KeepReference/);

protected:

    virtual uint8_t cpu_read_data(mem_addr_t);
    virtual void cpu_write_data(mem_addr_t, uint8_t);

};


class ArchAVR_Device : public Device /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_device.h"
%End

public:

    enum FlashSection /BaseType=IntEnum/ {
        Section_AppRWW        /PyName=AppRWW/,
        Section_AppNRWW       /PyName=AppNRWW/,
        Section_Boot          /PyName=Boot/
    };

    enum FlashSectionFlags /BaseType=IntFlag/ {
        Access_RWW            /PyName=RWW/,
        Access_IntDisabled    /PyName=IntDisabled/,
    };

    ArchAVR_Device(const ArchAVR_DeviceConfig& /KeepReference/);

    SIP_PYOBJECT __str__();
    %MethodCode
        sipRes = PyUnicode_FromFormat("Device(%s) [C=%lld, S=%d, F=%u]",
                                      sipCpp->config().name,
                                      sipCpp->cycle(),
                                      sipCpp->state(),
                                      sipCpp->frequency());
    %End

};
