/*
 * device.sip
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

%ModuleCode
#include "core/sim_device.h"

enum {
    Crash_PC_Overflow = CRASH_PC_OVERFLOW,
    Crash_SP_Overflow = CRASH_SP_OVERFLOW,
    Crash_Bad_CPU_IO = CRASH_BAD_CPU_IO,
    Crash_Bad_CTL_IO = CRASH_BAD_CTL_IO,
    Crash_Invalid_Opcode = CRASH_INVALID_OPCODE,
    Crash_Invalid_Config = CRASH_INVALID_CONFIG,
    Crash_Flash_Addr_Overflow = CRASH_FLASH_ADDR_OVERFLOW,
    Crash_Access_Refused = CRASH_ACCESS_REFUSED,
};

%End

enum CoreCrashCodes {
    Crash_PC_Overflow,
    Crash_SP_Overflow,
    Crash_Bad_CPU_IO,
    Crash_Bad_CTL_IO,
    Crash_Invalid_Opcode,
    Crash_Invalid_Config,
    Crash_Flash_Addr_Overflow,
    Crash_Access_Refused,
};


class Device /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_device.h"
%End

public:

    enum State {
        State_Limbo         /PyName=Limbo/,
        State_Ready         /PyName=Ready/,
        State_Running       /PyName=Running/,
        State_Sleeping      /PyName=Sleeping/,
        State_Halted        /PyName=Halted/,
        State_Reset         /PyName=Reset/,
        State_Break         /PyName=Break/,
        State_Done          /PyName=Done/,
        State_Crashed       /PyName=Crashed/,
        State_Destroying    /PyName=Destroying/,
    };

    enum ResetFlag /BaseType=IntFlag/ {
        Reset_PowerOn       /PyName=PowerOn/,
        Reset_WDT           /PyName=WDT/,
        Reset_BOD           /PyName=BOD/,
        Reset_SW            /PyName=SW/,
        Reset_Ext           /PyName=Ext/,
        Reset_Halt          /PyName=Halt/,
    };

    enum Option /BaseType=IntFlag/ {
        Option_ResetOnPinShorting    /PyName=ResetOnPinShorting/,
        Option_IgnoreBadCpuIO        /PyName=IgnoreBadCpuIO/,
        Option_IgnoreBadCpuLPM       /PyName=IgnoreBadCpuLPM/,
        Option_DisablePseudoSleep    /PyName=DisablePseudoSleep/,
        Option_InfiniteLoopDetect    /PyName=InfiniteLoopDetect/,
    };

    Device(Core&, const DeviceConfiguration&);

    Core& core() const;

    void set_option(Device::Option, bool);
    bool test_option(Device::Option) const;

    const DeviceConfiguration& config() const /NoCopy/;
    Device::State state() const;
    cycle_count_t cycle() const;
    SleepMode sleep_mode() const;
    unsigned long frequency() const;

    void init(CycleManager&)
        [bool(CycleManager&)];
    %MethodCode
        if (!sipCpp->init(*a0)) {
            PyErr_SetString(PyExc_Exception, "Device initialisation failed");
            return SIP_NULLPTR;
        }
    %End

    void load_firmware(const Firmware&)
        [bool(const Firmware&)];
    %MethodCode
        if (!sipCpp->load_firmware(*a0)) {
            PyErr_SetString(PyExc_Exception, "Device firmware load failed");
            return SIP_NULLPTR;
        }
    %End

    void reset(int = Device::Reset_PowerOn);

    cycle_count_t exec_cycle();

    void attach_peripheral(Peripheral& /Transfer/);
    void add_ioreg_handler(reg_addr_t, IO_RegHandler&, uint8_t = 0x00);
    void add_ioreg_handler(const regbit_t&, IO_RegHandler&, bool = false);
    Peripheral* find_peripheral(const char*);
    Peripheral* find_peripheral(ctl_id_t);

    SIP_PYTUPLE ctlreq(ctl_id_t, ctlreq_id_t, ctlreq_data_t* = NULL) /TypeHint="Tuple[bool, ctlreq_data_t]"/
        [bool(ctl_id_t, ctlreq_id_t, ctlreq_data_t*)];
    %MethodCode
        ctlreq_data_t* d;
        PyObject* transferObj;
        if (a2) {
            d = a2;
            transferObj = SIP_NULLPTR;
        } else {
            d = new ctlreq_data_t();
            transferObj = Py_None;
        }
        bool status = sipCpp->ctlreq(a0, a1, d);
        sipRes = sipBuildResult(0, "(bD)", status, d, sipType_ctlreq_data_t, transferObj);
    %End

    CycleManager* cycle_manager();

    Pin* find_pin(const char*);
    Pin* find_pin(pin_id_t);

    LogHandler& log_handler();
    Logger& logger();

    void crash(uint16_t, const char*);

};
