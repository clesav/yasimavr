/*
 * sim_device.h
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

#ifndef __YASIMAVR_DEVICE_H__
#define __YASIMAVR_DEVICE_H__

#include "sim_core.h"
#include "sim_cycle_timer.h"
#include "sim_peripheral.h"
#include <string>
#include <vector>
#include <map>

class AVR_DeviceConfiguration;
class AVR_Firmware;
class AVR_Interrupt;
class AVR_IO_Console;
enum class AVR_SleepMode;

//=======================================================================================
//Crash codes definition

#define CRASH_PC_OVERFLOW			0x01
#define CRASH_SP_OVERFLOW			0x02
#define CRASH_BAD_CPU_IO			0x03
#define CRASH_BAD_CTL_IO			0x04
#define CRASH_INVALID_OPCODE		0x05
#define CRASH_INVALID_CONFIG		0x06
#define CRASH_FLASH_ADDR_OVERFLOW	0x07


//=======================================================================================
/*
 * Abstract logger definition
 */
class DLL_EXPORT AVR_DeviceLogger {

public:

	enum {
		LOG_NONE = 0,
		LOG_OUTPUT,
		LOG_ERROR,
		LOG_WARNING,
		LOG_DEBUG,
		LOG_TRACE,
	};

	virtual ~AVR_DeviceLogger() {}

	virtual void set_level(int level) = 0;

	virtual void log(const int level, const char* format, ...) = 0;

};

//Macro definition for logging
#define ERROR_LOG(logger, format, ...) (logger).log(AVR_DeviceLogger::LOG_ERROR, "ERR: " format, __VA_ARGS__)
#define WARNING_LOG(logger, format, ...) (logger).log(AVR_DeviceLogger::LOG_WARNING, "WNG: " format, __VA_ARGS__)
#define DEBUG_LOG(logger, format, ...) (logger).log(AVR_DeviceLogger::LOG_DEBUG, "DBG: " format, __VA_ARGS__)

//Default logger that writes to stdout
extern DLL_EXPORT AVR_DeviceLogger& AVR_StandardLogger;


//=======================================================================================
/*
 * Generic AVR device definition, holding all the data about a MCU
 */
class DLL_EXPORT AVR_Device {

	friend class AVR_DeviceDebugProbe;

public:
    
    enum State {
    	State_Limbo 		= 0x00,
		State_Ready			= 0x10,
		State_Running 		= 0x21,
		State_Sleeping		= 0x31,
		State_Reset			= 0x40,
		State_Stopped		= 0x50,
		State_Break			= 0x60,
		State_Done			= 0x70,
		State_Crashed		= 0x80,
		State_Destroying	= 0xFF,
    };
	
	enum ResetFlag {
		Reset_PowerOn = 0x01,
		Reset_WDT	  = 0x02,
		Reset_BOD	  = 0x04,
		Reset_SW	  = 0x08,
		Reset_Ext	  = 0x10,
	};

    //These options are to be used with set_option() and test_option() to alter
    //the behaviour of the simulation
	enum Option {
		//By default, the simulation will halt if a pin shorting is detected.
		//If this option is set, it will instead trigger a MCU reset and the simulation will carry on.
		Option_ResetOnPinShorting	= 0x01,
		//By default the simulation will halt if the CPU writes a non-zero value to an
		//invalid I/O address (either it doesn't exist in the MCU model or is not supported by the simulator)
		//If this option is set, the write operation will be ignored and the simulation will carry on.
		//Note: read operations to invalid I/O addresses by the CPU always succeed and return 0.
		Option_IgnoreBadCpuIO		= 0x02,
		Option_IgnoreBadCpuLPM		= 0x04,
		//This option allows to disable the pseudo-sleep (triggered by a "rjmp .-2" instruction)
		Option_DisablePseudoSleep	= 0x08,
		//This option exits the simloop when the device enters sleep or a infinite
		//loop with GIE cleared. It is enabled by default.
		Option_InfiniteLoopDetect	= 0x10,
	};

    AVR_Device(AVR_Core& core, const AVR_DeviceConfiguration& config);
    virtual ~AVR_Device();
    
    AVR_Core& core() const;

    void set_option(Option option, bool value);
    bool test_option(Option option) const;

    const AVR_DeviceConfiguration& config() const;
    State state() const;
    cycle_count_t cycle() const;
    AVR_SleepMode sleep_mode() const; //Returns one of AVR_SleepMode enum values
    uint32_t frequency() const;

    //Init should be called just after constructing the device to allows all peripherals
    //to allocate resources and connect signals
    //Returns true on success or false on failure
    bool init(AVR_CycleManager& cycle_manager);

    //Loads a firmware object into the flash and loads the parameters in the .mcu section
    bool load_firmware(const AVR_Firmware& firmware);

    //Simulates a MCU reset
    void reset(uint8_t reset_flags = Reset_PowerOn);

    //Executes one instruction cycle
    //The returned value is the duration of the instruction in cycles
    cycle_count_t exec_cycle();

    //Attach a peripheral to the device. The peripheral will be owned by the device and will
    //be destroyed alongside
    void attach_peripheral(AVR_Peripheral* ctl);

    void add_ioreg_handler(reg_addr_t addr, AVR_IO_RegHandler* handler, uint8_t ro_mask=0x00);
    void add_ioreg_handler(regbit_t rb, AVR_IO_RegHandler* handler, bool readonly=false);
    AVR_Peripheral* find_peripheral(uint32_t id);
    bool ctlreq(uint32_t id, uint16_t req, ctlreq_data_t* reqdata = nullptr);

    //Helpers for the peripheral timers
    void add_cycle_timer(AVR_CycleTimer* timer, cycle_count_t when);
    void remove_cycle_timer(AVR_CycleTimer* timer);
    void reschedule_cycle_timer(AVR_CycleTimer* timer, cycle_count_t when);
    void pause_cycle_timer(AVR_CycleTimer* timer);
    void resume_cycle_timer(AVR_CycleTimer* timer);
    AVR_CycleManager& cycle_manager();

    AVR_Pin* find_pin(const char* name);
    AVR_Pin* find_pin(uint32_t id);

    void set_logger(AVR_DeviceLogger& logger);
    AVR_DeviceLogger& logger() const;

    void crash(uint16_t reason, const char* text);

protected:

    virtual bool core_ctlreq(uint16_t req, ctlreq_data_t* reqdata);

private:

    AVR_Core& m_core;
    const AVR_DeviceConfiguration& m_config;
    uint32_t m_options;
    State m_state;
    uint32_t m_frequency;
    AVR_SleepMode m_sleep_mode;
    AVR_DeviceDebugProbe* m_debugger;
    AVR_DeviceLogger *m_logger;
    std::vector<AVR_Peripheral*> m_peripherals;
    std::map<uint32_t, AVR_Pin*> m_pins;
    AVR_CycleManager* m_cycle_manager;
    AVR_IO_Console* m_console;
	uint8_t m_reset_flags;

    std::string& name_from_pin(AVR_Pin* pin);

    void set_state(State state);

};

inline const AVR_DeviceConfiguration& AVR_Device::config() const
{
	return m_config;
}

inline AVR_Device::State AVR_Device::state() const
{
	return m_state;
}

inline cycle_count_t AVR_Device::cycle() const
{
	return m_cycle_manager ? m_cycle_manager->cycle() : INVALID_CYCLE;
}

inline AVR_Core& AVR_Device::core() const
{
	return m_core;
}

inline AVR_SleepMode AVR_Device::sleep_mode() const
{
	return m_sleep_mode;
}

inline uint32_t AVR_Device::frequency() const
{
	return m_frequency;
}

inline void AVR_Device::set_state(State state)
{
	m_state = state;
}

inline AVR_DeviceLogger& AVR_Device::logger() const
{
	return *m_logger;
}

inline AVR_CycleManager& AVR_Device::cycle_manager()
{
	return *m_cycle_manager;
}

inline void AVR_Device::add_cycle_timer(AVR_CycleTimer* timer, cycle_count_t when)
{
	m_cycle_manager->add_cycle_timer(timer, when);
}

inline void AVR_Device::remove_cycle_timer(AVR_CycleTimer* timer)
{
	m_cycle_manager->remove_cycle_timer(timer);
}

inline void AVR_Device::reschedule_cycle_timer(AVR_CycleTimer* timer, cycle_count_t when)
{
	m_cycle_manager->reschedule_cycle_timer(timer, when);
}

inline void AVR_Device::pause_cycle_timer(AVR_CycleTimer* timer)
{
	m_cycle_manager->pause_cycle_timer(timer);
}

inline void AVR_Device::resume_cycle_timer(AVR_CycleTimer* timer)
{
	m_cycle_manager->resume_cycle_timer(timer);
}

#endif //__YASIMAVR_DEVICE_H__
