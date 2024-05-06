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
#define CRASH_ACCESS_REFUSED        0x08


//=======================================================================================
/**
   \brief Basic AVR device model.

   This is the top-level object for a AVR MCU simulation model.
 */
class AVR_CORE_PUBLIC_API Device {

    friend class DeviceDebugProbe;

public:

    /**
       Device model state enum.
     */
    enum State {
        State_Limbo         = 0x00, //!< Device constructed but not yet initialised
        State_Ready         = 0x10, //!< Device initialised but no firmware loaded yet
        State_Running       = 0x21, //!< Device executing the firmware
        State_Sleeping      = 0x31, //!< Device in sleep mode
        State_Halted        = 0x41, //!< CPU halted but peripherals are running normally
        State_Reset         = 0x50, //!< Device being reset (taken into account at the next cycle)
        State_Break         = 0x60, //!< Device halted by a BREAK instruction
        State_Done          = 0x70, //!< Final state without any error
        State_Crashed       = 0x80, //!< Final state with error
        State_Destroying    = 0xFF, //!< Transiting state during destruction
    };

    /**
       Reset source enum
     */
    enum ResetFlag {
        Reset_PowerOn = 0x00000001, //!< Power-On reset source
        Reset_WDT     = 0x00000002, //!< Watchdog Timer reset source
        Reset_BOD     = 0x00000004, //!< Brown-out Detector reset source
        Reset_SW      = 0x00000008, //!< Software reset source
        Reset_Ext     = 0x00000010, //!< External pin reset source
        Reset_Halt    = 0x00010000,
    };

    /**
       Device option enum

       These options are to be used with set_option() and test_option() to alter
       the behaviour of the simulation model.
     */
    enum Option {
        ///By default, the device will crash if a pin shorting is detected.
        ///If this option is set, it will instead simulate a BOD-triggered MCU reset.
        Option_ResetOnPinShorting   = 0x01,

        /**
           By default the device will crash on the following CPU I/O access errors:
              - Reading from a unallocated register,
              - Writing to a unallocated register,
              - Changing the value of a read-only field,
              - Writing a '1' to an unused bit.

           If this option is set, these errors will be ignored.
           \sa Core::cpu_read_ioreg, Core::cpu_write_ioreg
         */
        Option_IgnoreBadCpuIO       = 0x02,

        /**
           By default the device will crash if the CPU reads an unprogrammed address of the flash.
           If this option is set, the operation will succeed.
        */
        Option_IgnoreBadCpuLPM      = 0x04,

        ///This option disables the pseudo-sleep mode.
        Option_DisablePseudoSleep   = 0x08,

        /**
           This option makes the simulation loop exit when the device enters a sleep mode
           or an infinite loop instruction ("rjmp .-2") with the GIE bit cleared.
           It is set by default.
         */
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

    bool init(CycleManager& cycle_manager);

    bool load_firmware(const Firmware& firmware);

    void reset(int reset_flags = Reset_PowerOn);

    cycle_count_t exec_cycle();

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

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

protected:

    virtual bool core_ctlreq(ctlreq_id_t req, ctlreq_data_t* reqdata);

    virtual bool program(const Firmware& firmware);

    void erase_peripherals();

    virtual flash_addr_t reset_vector();

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
