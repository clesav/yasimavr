/*
 * sim_debug.h
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

#ifndef __YASIMAVR_DEBUG_H__
#define __YASIMAVR_DEBUG_H__

#include "sim_types.h"
#include "sim_device.h"

YASIMAVR_BEGIN_NAMESPACE

//=======================================================================================
/*
 * DeviceDebugProbe is a facility to access the inner state of a Device and its core
 * to debug the execution of a firmware.
 * It provides:
 *  - function to read/write directly into CPU registers, I/O registers, flash, data space
 *  - soft breakpoints
 *  - data watchpoints
 *  - device reset and state change
 */
class AVR_CORE_PUBLIC_API DeviceDebugProbe {

public:

    enum WatchpointFlags {
        //Event flags
        Watchpoint_Write = 0x01,
        Watchpoint_Read = 0x02,
        //Action flags
        Watchpoint_Signal = 0x10,
        Watchpoint_Break = 0x20,
    };

    DeviceDebugProbe();
    explicit DeviceDebugProbe(Device& device);
    DeviceDebugProbe(const DeviceDebugProbe& probe);
    //Destructor: ensures the probe is detached
    ~DeviceDebugProbe();

    Device* device() const;

    //Attaches the probe to a device, allowing access to its internals
    void attach(Device& device);
    //Attaches this to the same device as the argument
    void attach(DeviceDebugProbe& probe);
    //Detaches the probe from the device. it MUST be called before
    //destruction.
    void detach();
    bool attached() const;

    void reset_device() const;
    void set_device_state(Device::State state) const;

    //Access to general purpose CPU registers
    void write_gpreg(unsigned int num, uint8_t value) const;
    uint8_t read_gpreg(unsigned int num) const;

    //Access to the SREG register
    void write_sreg(uint8_t value) const;
    uint8_t read_sreg() const;

    //Access to the Stack Pointer
    void write_sp(mem_addr_t value) const;
    mem_addr_t read_sp() const;

    //Access to the Program Counter
    void write_pc(flash_addr_t value) const;
    flash_addr_t read_pc() const;

    //Access to I/O registers. From the peripherals point of view, it's
    //the same as a CPU access
    void write_ioreg(reg_addr_t addr, uint8_t value) const;
    uint8_t read_ioreg(reg_addr_t addr) const;

    //Access to the flash memory
    void write_flash(flash_addr_t addr, const uint8_t* buf, flash_addr_t len) const;
    flash_addr_t read_flash(flash_addr_t addr, uint8_t* buf, flash_addr_t len) const;

    //Access to the data space
    void write_data(mem_addr_t addr, const uint8_t* buf, mem_addr_t len) const;
    void read_data(mem_addr_t addr, uint8_t* buf, mem_addr_t len) const;

    //Breakpoint management
    void insert_breakpoint(flash_addr_t addr);
    void remove_breakpoint(flash_addr_t addr);

    //Watchpoint management
    void insert_watchpoint(mem_addr_t addr, mem_addr_t len, int flags);
    void remove_watchpoint(mem_addr_t addr, int flags);
    Signal& watchpoint_signal();

    //Callbacks from the CPU for notifications
    void _cpu_notify_data_read(mem_addr_t addr, uint8_t value);
    void _cpu_notify_data_write(mem_addr_t addr, uint8_t value);
    void _cpu_notify_jump(flash_addr_t addr);
    void _cpu_notify_call(flash_addr_t addr);
    void _cpu_notify_ret();

    DeviceDebugProbe& operator=(const DeviceDebugProbe& probe);

private:

    struct watchpoint_t {
        mem_addr_t addr;
        mem_addr_t len;
        int flags;
    };

    //Pointer to the device this is attached to.
    Device* m_device;
    //Pointer to the primary probe. If null, this is primary.
    DeviceDebugProbe* m_primary;
    //Vector of secondary probes.
    std::vector<DeviceDebugProbe*> m_secondaries;
    //Mapping containers PC => breakpoint
    std::map<flash_addr_t, breakpoint_t> m_breakpoints;
    //Mapping containers mem address => watchpoint
    std::map<mem_addr_t, watchpoint_t> m_watchpoints;
    //Signal for watchpoint notification
    Signal m_wp_signal;

    void notify_watchpoint(watchpoint_t& wp, int event, mem_addr_t addr, uint8_t value);

};

inline Device* DeviceDebugProbe::device() const
{
    return m_device;
}

inline bool DeviceDebugProbe::attached() const
{
    return !!m_device;
}

inline Signal& DeviceDebugProbe::watchpoint_signal()
{
    return m_wp_signal;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_DEBUG_H__
