/*
 * arch_avr_device.sip
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

struct AVR_ArchAVR_CoreConfig : public AVR_CoreConfiguration {
%TypeHeaderCode
#include "arch_avr_device.h"
%End
};

struct AVR_ArchAVR_DeviceConfig : public AVR_DeviceConfiguration /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_device.h"
%End
};


class AVR_ArchAVR_Core : public AVR_Core /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_device.h"
%End

public:

    enum AVR_ArchAVR_NVM /BaseType=IntEnum/ {
        NVM_EEPROM          /PyName=EEPROM/,
    };

    AVR_ArchAVR_Core(const AVR_ArchAVR_CoreConfig& /KeepReference/);

protected:

    virtual uint8_t cpu_read_data(mem_addr_t);
    virtual void cpu_write_data(mem_addr_t, uint8_t);

};


class AVR_ArchAVR_Device : public AVR_Device /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_device.h"
%End

public:

    AVR_ArchAVR_Device(const AVR_ArchAVR_DeviceConfig& /KeepReference/);

    SIP_PYOBJECT __str__();
    %MethodCode
        sipRes = PyUnicode_FromFormat("Device(%s) [C=%lld, S=%d, F=%u]",
                                      sipCpp->config().name,
                                      sipCpp->cycle(),
                                      sipCpp->state(),
                                      sipCpp->frequency());
    %End

};
