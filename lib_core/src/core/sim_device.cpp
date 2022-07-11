/*
 * sim_device.cpp
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

#include "sim_device.h"
#include "sim_firmware.h"
#include "sim_sleep.h"
#include "sim_cycle_timer.h"
#include "sim_debug.h"
#include "../ioctrl_common/sim_vref.h"
#include <cstring>
#include <cstdarg>

//=======================================================================================

class StandardDeviceLogger : public AVR_DeviceLogger {
public:
	StandardDeviceLogger() : m_level(LOG_ERROR) {}

	virtual void set_level(int level) override
	{
		m_level = level;
	}

	virtual void log(const int level, const char* format, ...) override
	{
		if (level > m_level) return;

		std::FILE* f = stdout; //level == LOG_OUTPUT ? stdout : stderr;

		std::va_list args;
		va_start(args, format);
		vfprintf(f, format, args);
		fprintf(f, "\n");
		va_end(args);
		fflush(f);
	}

private:
	int m_level;
};

static StandardDeviceLogger _std_logger;
AVR_DeviceLogger& AVR_StandardLogger = _std_logger;


//=======================================================================================
/*
 * Pseudo-peripheral that allows to use any register to output text to the simulator logger
 * Characters written by the CPU to this register are stored in a buffer and flushed to the
 * device logger on a newline character.
 */
class AVR_IO_Console : public AVR_Peripheral {

public:

	AVR_IO_Console();

	void set_register(reg_addr_t reg);
	virtual void reset() override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

	reg_addr_t m_reg_console;
	std::string m_buf;

};

AVR_IO_Console::AVR_IO_Console()
:AVR_Peripheral(AVR_ID('C', 'S', 'L', 'E'))
,m_reg_console(0)
{}

void AVR_IO_Console::set_register(reg_addr_t reg)
{
	if (!m_reg_console) {
		m_reg_console = reg;
		if (reg)
			device()->add_ioreg_handler(reg, this);
	}
}

void AVR_IO_Console::reset()
{
	if (m_buf.size())
		device()->logger().log(AVR_DeviceLogger::LOG_WARNING, "Console output lost by reset");
	m_buf.clear();
}

void AVR_IO_Console::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
	if (addr == m_reg_console) {
		if (data.value == '\n') {
			char s[20];
			sprintf(s, "[%llu] ", device()->cycle());
			m_buf.insert(0, s);
			device()->logger().log(AVR_DeviceLogger::LOG_OUTPUT, m_buf.c_str());
			m_buf.clear();
		} else {
			m_buf += (char) data.value;
		}
	}
}


//=======================================================================================

AVR_Device::AVR_Device(AVR_Core& core, const AVR_DeviceConfiguration& config)
:m_core(core)
,m_config(config)
,m_options(Option_InfiniteLoopDetect)
,m_state(State_Limbo)
,m_frequency(0)
,m_sleep_mode(AVR_SleepMode::Active)
,m_debugger(nullptr)
,m_logger(&_std_logger)
,m_cycle_manager(nullptr)
,m_reset_flags(0)
{
	//Allocate the pin array
	for (int i = 0; i < config.pins.size(); ++i) {
		uint32_t id = str_to_id(config.pins[i]);
		AVR_Pin* pin = new AVR_Pin(id);
		m_pins[id] = pin;
	}

	//Allocate the console peripheral
	m_console = new AVR_IO_Console();
	attach_peripheral(m_console);
}

AVR_Device::~AVR_Device()
{
	m_state = State_Destroying;

	//Destroys all the peripherals, last attached first destroyed.
	for (auto per_it = m_peripherals.rbegin(); per_it != m_peripherals.rend(); ++per_it) {
		AVR_Peripheral* per = *per_it;
		delete per;
	}

	//Destroys the device pins.
	for (auto it = m_pins.begin(); it != m_pins.end(); ++it) {
		AVR_Pin* pin = it->second;
		delete pin;
	}
}


//=======================================================================================

void AVR_Device::set_option(Option option, bool value)
{
	if (value)
		m_options |= option;
	else
		m_options &= ~option;
}

bool AVR_Device::test_option(Option option) const
{
	return m_options & option;
}


//=======================================================================================

bool AVR_Device::init(AVR_CycleManager& cycle_manager)
{
	if (m_state != State_Limbo)
		return false;

	m_cycle_manager = &cycle_manager;

	DEBUG_LOG(*m_logger, "Initialisation of %s core", m_config.name);
	if (!m_core.init(*this)) {
		ERROR_LOG(*m_logger, "Initialisation of %s core failed.", m_config.name);
		return false;
	}

	for (auto per : m_peripherals) {
		const char* per_name = id_to_str(per->id()).c_str();
		DEBUG_LOG(*m_logger, "Initialisation of peripheral '%s'", per_name);
		if (!per->init(*this)) {
			ERROR_LOG(*m_logger,
					  "Initialisation of peripheral '%s' of %s failed.",
					  per_name,
					  m_config.name);
			return false;
		}
	}

	m_state = State_Ready;
	reset();

	DEBUG_LOG(*m_logger, "Initialisation of device '%s' complete", m_config.name);

	return true;
}

void AVR_Device::reset(uint8_t reset_flag)
{
	DEBUG_LOG(*m_logger, "Device reset", "");
	
	m_reset_flags |= reset_flag;

	m_core.reset();

	for (auto per : m_peripherals)
		per->reset();
	
	if (m_state > State_Running)
		m_state = State_Running;

	m_reset_flags = 0;
	m_sleep_mode = AVR_SleepMode::Active;
}

bool AVR_Device::load_firmware(const AVR_Firmware& firmware)
{
	if (m_state != State_Ready) {
		ERROR_LOG(*m_logger, "Firmware load: Device not ready", "");
		return false;
	}

	if (!program(firmware))
		return false;

	//Stores the frequency. Compulsory.
	if (!firmware.frequency) {
		ERROR_LOG(*m_logger, "Firmware load: MCU frequency not defined", "");
		return false;
	}
	m_frequency = firmware.frequency;

	//Send the power supply voltage from the firmware to the VREF controller (if it exists)
	bool analog_ok = false;
	if (firmware.vcc > 0.0) {
		ctlreq_data_t reqdata = { .data = firmware.vcc, .index = AVR_IO_VREF::Source_Ext_VCC, };
		analog_ok = ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_SET, &reqdata);
		if (analog_ok) {
			//Send the analog voltage reference from the firmware to the VREF controller
			reqdata.index = AVR_IO_VREF::Source_Ext_AREF;
			reqdata.data = firmware.aref;
			ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_SET, &reqdata);
		} else {
			ERROR_LOG(*m_logger, "Firmware load: Unable to set VCC, analog features are unusable.", "");
		}
	} else {
		DEBUG_LOG(*m_logger, "Firmware load: VCC not defined in the firmware, analog features are unusable.", "");
	}

	//Set the console register
	m_console->set_register(firmware.console_register);

	m_state = State_Running;

	return true;
}

bool AVR_Device::program(const AVR_Firmware& firmware)
{
	if (!firmware.has_memory("flash")) {
		ERROR_LOG(*m_logger, "Firmware load: No program to load", "");
		return false;
	}
	else if (firmware.load_memory("flash", m_core.m_flash)) {
		DEBUG_LOG(*m_logger, "Loaded %d bytes of flash", firmware.flashsize());
	} else {
		ERROR_LOG(*m_logger, "Firmware load: The flash does not fit", "");
		return false;
	}
	
	if (firmware.has_memory("fuse")) {
		if (!firmware.load_memory("fuse", m_core.m_fuses)) {
			DEBUG_LOG(*m_logger, "Firmware load: fuses loaded", "");
		} else {
			ERROR_LOG(*m_logger, "Firmware load: Error programming the fuses", "");
			return false;
		}
	}
	
	return true;
}

cycle_count_t AVR_Device::exec_cycle()
{
	if (!(m_state & 0x0F)) return 0;

	cycle_count_t cycle_delta;
	if (m_state == State_Running)
		cycle_delta = m_core.exec_cycle();
	else
		cycle_delta = 1;

	if (m_state == State_Reset)
		reset();

	return cycle_delta;
}


//=======================================================================================
//Management of I/O peripherals

void AVR_Device::attach_peripheral(AVR_Peripheral* ctl)
{
	if (!ctl) return;

	if (ctl->id() == AVR_IOCTL_INTR)
		m_core.m_intrctl = reinterpret_cast<AVR_InterruptController*>(ctl);

	m_peripherals.push_back(ctl);
}

bool AVR_Device::ctlreq(uint32_t id, uint16_t req, ctlreq_data_t* reqdata)
{
	if (id == AVR_IOCTL_CORE) {
		return core_ctlreq(req, reqdata);
	} else {
		for (auto per : m_peripherals) {
			if (id == per->id()) {
				return per->ctlreq(req, reqdata);
			}
		}

		WARNING_LOG(*m_logger,
					"Sending request but peripheral %s not found",
					id_to_str(id).c_str());
		return false;
	}
}

AVR_Peripheral* AVR_Device::find_peripheral(uint32_t id)
{
	for (auto per : m_peripherals) {
		if (per->id() == id)
			return per;
	}
	return nullptr;
}

void AVR_Device::add_ioreg_handler(reg_addr_t addr, AVR_IO_RegHandler* handler, uint8_t ro_mask)
{
	if (addr != R_SREG && addr > 0) {
		DEBUG_LOG(*m_logger, "Registering handler for I/O 0x%04X", addr);
		AVR_IO_Register* reg = m_core.get_ioreg(addr);
		reg->set_handler(handler, 0xFF, ro_mask);
	}
}

void AVR_Device::add_ioreg_handler(regbit_t rb, AVR_IO_RegHandler *handler, bool readonly)
{
	if (rb.addr != R_SREG && rb.addr > 0) {
		DEBUG_LOG(*m_logger, "Registering handler for I/O 0x%04X", rb.addr);
		AVR_IO_Register* reg = m_core.get_ioreg(rb.addr);
		reg->set_handler(handler, rb.mask, readonly ? rb.mask : 0x00);
	}
}

bool AVR_Device::core_ctlreq(uint16_t req, ctlreq_data_t* reqdata)
{
	if (req == AVR_CTLREQ_CORE_BREAK) {
		if (m_core.m_debug_probe) {
			WARNING_LOG(*m_logger, "Device break at PC=%04x", m_core.m_pc);
			m_state = State_Break;
		}
		return true;
	}

	else if (req == AVR_CTLREQ_CORE_SLEEP) {
		if (test_option(Option_InfiniteLoopDetect) && !m_core.m_sreg[SREG_I]) {
			//The device cannot get out of sleep or infinite loop if GIE=0.
			//If the detect option is enabled, we exit the sim loop.
			if ((AVR_SleepMode) reqdata->data.as_uint() == AVR_SleepMode::Pseudo)
				WARNING_LOG(*m_logger, "Device in infinite loop with GIE=0, stopping.", "");
			else
				WARNING_LOG(*m_logger, "Device going to sleep with GIE=0, stopping.", "");

			WARNING_LOG(*m_logger, "End of program at PC = 0x%04x", m_core.m_pc);
			m_state = State_Done;

		} else {

			m_state = State_Sleeping;
			m_sleep_mode = (AVR_SleepMode) reqdata->data.as_uint();

			if (m_sleep_mode == AVR_SleepMode::Pseudo)
				DEBUG_LOG(*m_logger, "Device going to pseudo sleep", "");
			else
				DEBUG_LOG(*m_logger, "Device going to sleep mode %s", SleepModeName(m_sleep_mode));

			for (auto ioctl : m_peripherals)
				ioctl->sleep(true, m_sleep_mode);
		}

		return true;
	}

	else if (req == AVR_CTLREQ_CORE_WAKEUP) {
		DEBUG_LOG(*m_logger, "Device waking up", "");
		for (auto per : m_peripherals)
			per->sleep(false, m_sleep_mode);

		m_state = State_Running;
		m_sleep_mode = AVR_SleepMode::Active;

		return true;
	}

	else if (req == AVR_CTLREQ_CORE_SHORTING) {
		ERROR_LOG(*m_logger, "Pin %s shorted", m_config.pins[reqdata->index]);
		if (m_options & Option_ResetOnPinShorting) {
			m_reset_flags |= Reset_BOD;
			m_state = State_Reset;
		} else {
			m_state = State_Stopped;
		}
		return true;
	}

	else if (req == AVR_CTLREQ_CORE_CRASH) {
		ERROR_LOG(*m_logger, "CPU crash, reason=%d", reqdata->index);
		WARNING_LOG(*m_logger, "End of program at PC = 0x%04x", m_core.m_pc);
		m_state = State_Crashed;
		return true;
	}
	
	else if (req == AVR_CTLREQ_CORE_RESET) {
		m_reset_flags |= reqdata->data.as_uint();
		m_state = State_Reset;
		WARNING_LOG(*m_logger, "MCU reset triggered, Flags = 0x%02x", m_reset_flags);
		return true;
	}
	
	else if (req == AVR_CTLREQ_CORE_RESET_FLAG) {
		reqdata->data = m_reset_flags;
		return true;
	}
	
	else if (req == AVR_CTLREQ_CORE_NVM) {
		reqdata->data = (void*) nullptr;
		if (reqdata->index == AVR_Core::NVM_Flash)
			reqdata->data = &(m_core.m_flash);
		else if (reqdata->index == AVR_Core::NVM_Fuses)
			reqdata->data = &(m_core.m_fuses);
		return true;
	}
	
	else if (req == AVR_CTLREQ_CORE_HALT) {
		if (m_state == State_Running && reqdata->data.as_uint()) {
			m_state = State_Halted;
			DEBUG_LOG(*m_logger, "Device halted", "");
		}
		else if (m_state == State_Halted && !reqdata->data.as_uint()) {
			m_state = State_Running;
			DEBUG_LOG(*m_logger, "Device resuming from halt", "");
		}
		return true;
	}
	
	return false;
}


//=======================================================================================
//Management of the logger and the debugger interface

void AVR_Device::set_logger(AVR_DeviceLogger& logger)
{
	m_logger = &logger;
}


//=======================================================================================
//Management of device pins

AVR_Pin* AVR_Device::find_pin(const char* name)
{
	return find_pin(str_to_id(name));
}

AVR_Pin* AVR_Device::find_pin(uint32_t id)
{
	auto search = m_pins.find(id);
	if (search == m_pins.end())
		return nullptr;
	else
		return search->second;
}


//=======================================================================================
//Management of the crashes

void AVR_Device::crash(uint16_t reason, const char* text)
{
	ERROR_LOG(*m_logger, "MCU crash, reason (code=%d) : %s", reason, text);
	WARNING_LOG(*m_logger, "End of program at PC = 0x%04x", m_core.m_pc);
	m_state = State_Crashed;
}
