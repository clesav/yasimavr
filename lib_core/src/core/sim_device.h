/*
 * sim_device.h
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

#ifndef __YASIMAVR_DEVICE_H__
#define __YASIMAVR_DEVICE_H__

#include "sim_core.h"
#include "sim_cycle_timer.h"
#include "sim_peripheral.h"
#include "sim_logger.h"
#include <string>
#include <vector>
#include <map>

YASIMAVR_BEGIN_NAMESPACE

class DeviceConfiguration;
class Firmware;
class Interrupt;
enum class SleepMode;


//=======================================================================================
//Crash codes definition

#define CRASH_PC_OVERFLOW           0x01
#define CRASH_SP_OVERFLOW           0x02
#define CRASH_BAD_CPU_IO            0x03
#define CRASH_BAD_CTL_IO            0x04
#define CRASH_INVALID_OPCODE        0x05
#define CRASH_INVALID_CONFIG        0x06
#define CRASH_FLASH_ADDR_OVERFLOW   0x07


//=======================================================================================
/*
 * Generic AVR device definition, holding all the data about a MCU
 */
class AVR_CORE_PUBLIC_API Device {

    friend class DeviceDebugProbe;

public:

    enum State {
        State_Limbo         = 0x00,
        State_Ready         = 0x10,
        State_Running       = 0x21,
        State_Sleeping      = 0x31,
        State_Halted        = 0x41,
        State_Reset         = 0x50,
        State_Break         = 0x60,
        State_Done          = 0x70,
        State_Crashed       = 0x80,
        State_Destroying    = 0xFF,
    };

    enum ResetFlag {
        Reset_PowerOn = 0x00000001,
        Reset_WDT     = 0x00000002,
        Reset_BOD     = 0x00000004,
        Reset_SW      = 0x00000008,
        Reset_Ext     = 0x00000010,
        Reset_Halt    = 0x00010000,
    };

    //These options are to be used with set_option() and test_option() to alter
    //the behaviour of the simulation
    enum Option {
        //By default, the simulation will halt if a pin shorting is detected.
        //If this option is set, it will instead trigger a MCU reset and the simulation will carry on.
        Option_ResetOnPinShorting   = 0x01,
        //By default the simulation will halt if the CPU writes a non-zero value to an
        //invalid I/O address (either it doesn't exist in the MCU model or is not supported by the simulator)
        //If this option is set, the write operation will be ignored and the simulation will carry on.
        //Note: read operations to invalid I/O addresses by the CPU always succeed and return 0.
        Option_IgnoreBadCpuIO       = 0x02,
        Option_IgnoreBadCpuLPM      = 0x04,
        //This option allows to disable the pseudo-sleep (triggered by a "rjmp .-2" instruction)
        Option_DisablePseudoSleep   = 0x08,
        //This option exits the simloop when the device enters sleep or a infinite
        //loop with GIE cleared. It is enabled by default.
        Option_InfiniteLoopDetect   = 0x10,
    };

    Device(Core& core, const DeviceConfiguration& config);
    virtual ~Device();

    Core& core() const;

    void set_option(Option option, bool value);
    bool test_option(Option option) const;

    const DeviceConfiguration& config() const;
    State state() const;
    cycle_count_t cycle() const;
    SleepMode sleep_mode() const; //Returns one of SleepMode enum values
    unsigned long frequency() const;

    //Init should be called just after constructing the device to allows all peripherals
    //to allocate resources and connect signals
    //Returns true on success or false on failure
    bool init(CycleManager& cycle_manager);

    //Loads a firmware object into the flash and loads the parameters in the .mcu section
    bool load_firmware(const Firmware& firmware);

    //Simulates a MCU reset
    void reset(int reset_flags = Reset_PowerOn);

    //Executes one instruction cycle
    //The returned value is the duration of the instruction in cycles
    cycle_count_t exec_cycle();

    //Attach a peripheral to the device. The peripheral will be owned by the device and will
    //be destroyed alongside
    void attach_peripheral(Peripheral& ctl);

    void add_ioreg_handler(reg_addr_t addr, IO_RegHandler& handler, uint8_t ro_mask=0x00);
    void add_ioreg_handler(const regbit_t& rb, IO_RegHandler& handler, bool readonly=false);
    Peripheral* find_peripheral(const char* name);
    Peripheral* find_peripheral(ctl_id_t id);
    bool ctlreq(ctl_id_t id, ctlreq_id_t req, ctlreq_data_t* reqdata = nullptr);

    //Helpers for the peripheral timers
    CycleManager* cycle_manager();

    Pin* find_pin(const char* name);
    Pin* find_pin(pin_id_t id);

    LogHandler& log_handler();
    Logger& logger();

    void crash(uint16_t reason, const char* text);

    //Disable copy semantics
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

protected:

    virtual bool core_ctlreq(ctlreq_id_t req, ctlreq_data_t* reqdata);

    //Loads the various memory area using the firmware data.
    //The basic implementation loads only the flash and the fuses, the rest
    //is the responsibility of architecture-specific implementations.
    virtual bool program(const Firmware& firmware);

    void erase_peripherals();

private:

    Core& m_core;
    const DeviceConfiguration& m_config;
    int m_options;
    State m_state;
    unsigned long m_frequency;
    SleepMode m_sleep_mode;
    DeviceDebugProbe* m_debugger;
    LogHandler m_log_handler;
    Logger m_logger;
    std::vector<Peripheral*> m_peripherals;
    std::map<pin_id_t, Pin*> m_pins;
    CycleManager* m_cycle_manager;
    int m_reset_flags;

    std::string& name_from_pin(Pin* pin);

    void set_state(State state);

};

inline const DeviceConfiguration& Device::config() const
{
    return m_config;
}

inline Device::State Device::state() const
{
    return m_state;
}

inline cycle_count_t Device::cycle() const
{
    return m_cycle_manager ? m_cycle_manager->cycle() : INVALID_CYCLE;
}

inline Core& Device::core() const
{
    return m_core;
}

inline SleepMode Device::sleep_mode() const
{
    return m_sleep_mode;
}

inline unsigned long Device::frequency() const
{
    return m_frequency;
}

inline void Device::set_state(State state)
{
    m_state = state;
}

inline LogHandler& Device::log_handler()
{
    return m_log_handler;
}

inline Logger& Device::logger()
{
    return m_logger;
}

inline CycleManager* Device::cycle_manager()
{
    return m_cycle_manager;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_DEVICE_H__
