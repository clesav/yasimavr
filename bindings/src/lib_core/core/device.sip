/*
 * device.sip
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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
};

%End

enum AVR_CoreCrashCodes {
	Crash_PC_Overflow,
	Crash_SP_Overflow,
	Crash_Bad_CPU_IO,
	Crash_Bad_CTL_IO,
	Crash_Invalid_Opcode,
	Crash_Invalid_Config,
	Crash_Flash_Addr_Overflow
};


//=======================================================================================

class AVR_DeviceLogger /Abstract/ {
%TypeHeaderCode
#include "core/sim_device.h"
%End

public:

	enum {
		LOG_NONE,
		LOG_OUTPUT,
		LOG_ERROR,
		LOG_WARNING,
		LOG_DEBUG,
		LOG_TRACE,
	};

	virtual ~AVR_DeviceLogger();

	virtual void set_level(int) = 0;
	virtual void log(const int, const char*, ...) = 0;

};


//=======================================================================================

AVR_DeviceLogger& AVR_StandardLogger;


class AVR_Device /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_device.h"
%End

public:

    enum State {
		State_Limbo         /PyName=Limbo/,
		State_Ready         /PyName=Ready/,
		State_Running       /PyName=Running/,
		State_Sleeping      /PyName=Sleeping/,
		State_Halted		/PyName=Halted/,
		State_Reset         /PyName=Reset/,
		State_Stopped       /PyName=Stopped/,
		State_Break         /PyName=Break/,
		State_Done          /PyName=Done/,
		State_Crashed       /PyName=Crashed/,
		State_Destroying    /PyName=Destroying/,
    };
    
   	enum ResetFlag {
		Reset_PowerOn,
		Reset_WDT,
		Reset_BOD,
		Reset_SW,
	};

	enum Option /BaseType=IntFlag/ {
		Option_ResetOnPinShorting,
		Option_IgnoreBadCpuIO,
		Option_IgnoreBadCpuLPM,
		Option_DisablePseudoSleep,
		Option_InfiniteLoopDetect
	};

    AVR_Device(AVR_Core&, const AVR_DeviceConfiguration&);
    virtual ~AVR_Device();
    
    AVR_Core& core() const;

    void set_option(AVR_Device::Option, bool);
    bool test_option(AVR_Device::Option) const;

	const AVR_DeviceConfiguration& config() const /NoCopy/;
    AVR_Device::State state() const;
    cycle_count_t cycle() const;
    AVR_SleepMode sleep_mode() const;
    uint32_t frequency() const;

    void init(AVR_CycleManager&)
        [bool(AVR_CycleManager&)];
    %MethodCode
        if (!sipCpp->init(*a0)) {
            PyErr_SetString(PyExc_Exception, "Device initialisation failed");
            return SIP_NULLPTR;
        }
    %End
    
    void load_firmware(const AVR_Firmware&)
        [bool(const AVR_Firmware&)];
    %MethodCode
        if (!sipCpp->load_firmware(*a0)) {
            PyErr_SetString(PyExc_Exception, "Device firmware load failed");
            return SIP_NULLPTR;
        }
    %End

    void reset(uint8_t = AVR_Device::Reset_PowerOn);
    
    cycle_count_t exec_cycle();

    void attach_peripheral(AVR_Peripheral* /Transfer/);
    void add_ioreg_handler(reg_addr_t, AVR_IO_RegHandler*, uint8_t = 0x00);
    void add_ioreg_handler(regbit_t, AVR_IO_RegHandler*, bool = false);
    AVR_Peripheral* find_peripheral(uint32_t);
    
    SIP_PYTUPLE ctlreq(uint32_t, uint16_t, ctlreq_data_t* = NULL) /TypeHint="Tuple[bool, ctlreq_data_t]"/
    	[bool(uint32_t, uint16_t, ctlreq_data_t*)];
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

    void add_cycle_timer(AVR_CycleTimer*, cycle_count_t);
    void remove_cycle_timer(AVR_CycleTimer*);
    void reschedule_cycle_timer(AVR_CycleTimer*, cycle_count_t);
	void pause_cycle_timer(AVR_CycleTimer*);
	void resume_cycle_timer(AVR_CycleTimer*);
	AVR_CycleManager& cycle_manager();

    AVR_Pin* find_pin(const char*);
    AVR_Pin* find_pin(uint32_t);

    void set_logger(AVR_DeviceLogger&);
    AVR_DeviceLogger& logger() const;

    void crash(uint16_t, const char*);  

};
