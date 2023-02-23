/*
 * sim_debug.cpp
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

#include "sim_debug.h"
#include "sim_device.h"
#include "sim_core.h"
#include <cstring>

#define ADJUST_ADDR_LEN(addr, len, end) \
    if ((addr) > (end))                    \
        (addr) = (end);                    \
    if (!(end))                            \
        (len) = 0;                         \
    else if ((len) > ((end) + 1 - (addr))) \
        (len) = ((end) + 1 - (addr));


//=======================================================================================

AVR_DeviceDebugProbe::AVR_DeviceDebugProbe()
:m_device(nullptr)
,m_primary(nullptr)
{}

AVR_DeviceDebugProbe::AVR_DeviceDebugProbe(AVR_Device& device)
:AVR_DeviceDebugProbe()
{
    attach(device);
}

AVR_DeviceDebugProbe::AVR_DeviceDebugProbe(const AVR_DeviceDebugProbe& probe)
:AVR_DeviceDebugProbe()
{
    *this = probe;
}

AVR_DeviceDebugProbe::~AVR_DeviceDebugProbe()
{
    detach();
}

AVR_DeviceDebugProbe& AVR_DeviceDebugProbe::operator=(const AVR_DeviceDebugProbe& other)
{
    if (other.attached())
        attach(*other.device());
    else
        detach();

    return *this;
}

void AVR_DeviceDebugProbe::attach(AVR_Device& device)
{
    if (m_device == &device)
        return;

    if (m_device)
        detach();

    AVR_Core& core = device.core();

    if (core.m_debug_probe) {
        //Attach this as secondary
        m_primary = core.m_debug_probe;
        m_primary->m_secondaries.push_back(this);
    } else {
        //Attach this as primary
        core.m_debug_probe = this;
    }

    m_device = &device;
}

void AVR_DeviceDebugProbe::attach(AVR_DeviceDebugProbe& probe)
{
    if (probe.attached())
        attach(*probe.device());
}

void AVR_DeviceDebugProbe::detach()
{
    if (m_primary) {
        //If this is a secondary probe, detach from the primary
        for (auto it = m_primary->m_secondaries.begin(); it != m_primary->m_secondaries.end(); ++it) {
            if (*it == this) {
                m_primary->m_secondaries.erase(it);
                break;
            }
        }
        m_primary = nullptr;

    } else if (m_device) {
        //if m_device is set and not m_primary, this is an attached primary probe

        if (m_secondaries.size()) {
            //If there is at least one secondary probe, the first one gets
            //promoted to primary role, in place of this. So this must hand over
            //all secondaries (except the first one), all breakpoints and all watchpoints.

            //Extract the secondary probe that gets promoted
            AVR_DeviceDebugProbe* new_primary = m_secondaries[0];
            m_secondaries.erase(m_secondaries.begin());

            //Promote to primary role
            m_device->core().m_debug_probe = new_primary;
            new_primary->m_primary = nullptr;

            //Hand over the other secondaries
            for (auto sec : m_secondaries)
                sec->m_primary = new_primary;
            new_primary->m_secondaries = std::move(m_secondaries);

            //Hand over the breakpoints and watchpoints
            new_primary->m_breakpoints = std::move(m_breakpoints);
            new_primary->m_watchpoints = std::move(m_watchpoints);

        } else {
            //If no secondary to promote, unregister from the core and destroy
            //all breakpoints and watchpoints

            m_device->core().m_debug_probe = nullptr;

            for (auto& [addr, bp] : m_breakpoints)
                m_device->core().dbg_remove_breakpoint(bp);
            m_breakpoints.clear();

            m_watchpoints.clear();

        }
    }

    m_device = nullptr;
}

void AVR_DeviceDebugProbe::reset_device() const
{
    if (m_device)
        m_device->reset();
}

void AVR_DeviceDebugProbe::set_device_state(AVR_Device::State state) const
{
    if (m_device)
        m_device->set_state(state);
}

void AVR_DeviceDebugProbe::write_gpreg(unsigned int num, uint8_t value) const
{
    if (m_device)
        m_device->core().m_regs[num] = value;
}

uint8_t AVR_DeviceDebugProbe::read_gpreg(unsigned int num) const
{
    if (!m_device) return 0;
    return m_device->core().m_regs[num];
}

void AVR_DeviceDebugProbe::write_sreg(uint8_t value) const
{
    if (m_device)
        m_device->core().write_sreg(value);
}

uint8_t AVR_DeviceDebugProbe::read_sreg() const
{
    if (!m_device) return 0;
    return m_device->core().read_sreg();
}

void AVR_DeviceDebugProbe::write_sp(uint16_t value) const
{
    if (m_device)
        m_device->core().write_sp(value);
}

uint16_t AVR_DeviceDebugProbe::read_sp() const
{
    if (!m_device) return 0;
    return m_device->core().read_sp();
}

void AVR_DeviceDebugProbe::write_pc(uint32_t value) const
{
    if (m_device)
        m_device->core().m_pc = value;
}

uint32_t AVR_DeviceDebugProbe::read_pc() const
{
    if (!m_device) return 0;
    return m_device->core().m_pc;
}

void AVR_DeviceDebugProbe::write_ioreg(reg_addr_t addr, uint8_t value) const
{
    if (!m_device) return;

    AVR_Core& core = m_device->core();
    const uint16_t iosize = core.config().ioend - core.config().iostart + 1;

    if (addr == R_SREG) {
        core.write_sreg(value);
    }
    else if (addr < iosize) {
        AVR_IO_Register *ioreg = core.m_ioregs[addr];
        if (ioreg)
            ioreg->cpu_write(addr, value);
    }
}

uint8_t AVR_DeviceDebugProbe::read_ioreg(reg_addr_t addr) const
{
    if (!m_device) return 0;

    AVR_Core& core = m_device->core();
    const uint16_t iosize = core.config().ioend - core.config().iostart + 1;

    if (addr == R_SREG) {
        return core.read_sreg();
    }
    else if (addr < iosize) {
        AVR_IO_Register *ioreg = core.m_ioregs[addr];
        if (ioreg)
            return ioreg->cpu_read(addr);
    }
    return 0;
}

void AVR_DeviceDebugProbe::write_flash(flash_addr_t addr, const uint8_t* buf, flash_addr_t len) const
{
    if (!m_device) return;

    AVR_Core& core = m_device->core();

    ADJUST_ADDR_LEN(addr, len, core.config().flashend);

    core.m_flash.dbg_write(buf, addr, len);
}

flash_addr_t AVR_DeviceDebugProbe::read_flash(flash_addr_t addr, uint8_t* buf, flash_addr_t len) const
{
    if (!m_device) return 0;

    AVR_Core& core = m_device->core();

    ADJUST_ADDR_LEN(addr, len, core.config().flashend);

    core.m_flash.dbg_read(buf, addr, len);

    return len;
}

void AVR_DeviceDebugProbe::write_data(mem_addr_t addr, const uint8_t* buf, mem_addr_t len) const
{
    if (!m_device) return;

    //We need to momentarily disable the crash triggered by bad I/O access
    //as we copy the I/O registers as a block
    bool badioopt = m_device->test_option(AVR_Device::Option_IgnoreBadCpuIO);
    m_device->set_option(AVR_Device::Option_IgnoreBadCpuIO, true);

    m_device->core().dbg_write_data(addr, buf, len);

    m_device->set_option(AVR_Device::Option_IgnoreBadCpuIO, badioopt);
}

void AVR_DeviceDebugProbe::read_data(mem_addr_t addr, uint8_t* buf, mem_addr_t len) const
{
    if (!m_device) return;

    //Same as write_data()
    bool badioopt = m_device->test_option(AVR_Device::Option_IgnoreBadCpuIO);
    m_device->set_option(AVR_Device::Option_IgnoreBadCpuIO, true);

    m_device->core().dbg_read_data(addr, buf, len);

    m_device->set_option(AVR_Device::Option_IgnoreBadCpuIO, badioopt);
}

void AVR_DeviceDebugProbe::insert_breakpoint(flash_addr_t addr)
{
    if (!m_device) return;

    if (m_primary) {
        m_primary->insert_breakpoint(addr);
    }
    else if (m_breakpoints.find(addr) == m_breakpoints.end()) {
        m_breakpoints[addr] = breakpoint_t{ .addr = addr };
        m_device->core().dbg_insert_breakpoint(m_breakpoints[addr]);
    }
}

void AVR_DeviceDebugProbe::remove_breakpoint(flash_addr_t addr)
{
    if (!m_device) return;

    if (m_primary) {
        m_primary->remove_breakpoint(addr);
        return;
    }

    auto search = m_breakpoints.find(addr);
    if (search != m_breakpoints.end()) {
        m_device->core().dbg_remove_breakpoint(search->second);
        m_breakpoints.erase(search);
    }
}

void AVR_DeviceDebugProbe::insert_watchpoint(mem_addr_t addr, mem_addr_t len, int flags)
{
    if (!m_device) return;

    if (m_primary) {
        m_primary->insert_watchpoint(addr, len, flags);
        return;
    }

    auto search = m_watchpoints.find(addr);
    if (search != m_watchpoints.end()) {
        //If the watchpoint is found, just OR the flags
        search->second.flags |= flags;
    } else {
        m_watchpoints[addr] = { addr, len, flags };
    }
}

void AVR_DeviceDebugProbe::remove_watchpoint(mem_addr_t addr, int flags)
{
    if (!m_device) return;

    if (m_primary) {
        m_primary->remove_watchpoint(addr, flags);
        return;
    }

    auto search = m_watchpoints.find(addr);
    if (search != m_watchpoints.end()) {
        //If the watchpoint exists, clear its flags
        search->second.flags &= ~flags;
        //If neither Read or Write event flags are left, destroy the watchpoint
        if (!(search->second.flags & (Watchpoint_Write | Watchpoint_Read)))
            m_watchpoints.erase(search);
    }
}

void AVR_DeviceDebugProbe::notify_watchpoint(watchpoint_t& wp, int event, mem_addr_t addr)
{
    //If the watchpoint flag is set to Break, halt the CPU, only if this is the primary probe
    if (!m_primary && (wp.flags & Watchpoint_Break)) {
        m_device->logger().wng("Device break on watchpoint A=%04x", addr);
        m_device->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_BREAK, nullptr);
    }

    //If the watchpoint flag is set to Signal, raise the signal
    if (wp.flags & Watchpoint_Signal)
        m_wp_signal.raise_u(event, wp.addr);

    //Notify the secondary probes
    for (auto secondary : m_secondaries)
        secondary->notify_watchpoint(wp, event, addr);
}

//Notification when the CPU reads from the RAM. Check if there's a watchpoint associated with the address.
void AVR_DeviceDebugProbe::_cpu_notify_data_read(mem_addr_t addr)
{
    for (auto& [_, wp] : m_watchpoints) {
        if (addr >= wp.addr && addr < (wp.addr + wp.len) && (wp.flags & Watchpoint_Read)) {
            notify_watchpoint(wp, Watchpoint_Read, addr);
            return;
        }
    }
}

//Notification when the CPU writes into the RAM. Check if there's a watchpoint associated with the address.
void AVR_DeviceDebugProbe::_cpu_notify_data_write(mem_addr_t addr)
{
    for (auto& [_, wp] : m_watchpoints) {
        if (addr >= wp.addr && addr < (wp.addr + wp.len) && (wp.flags & Watchpoint_Write)) {
            notify_watchpoint(wp, Watchpoint_Write, addr);
            return;
        }
    }
}

//For future use maybe
void AVR_DeviceDebugProbe::_cpu_notify_jump(flash_addr_t addr) {}
void AVR_DeviceDebugProbe::_cpu_notify_call(flash_addr_t addr) {}
void AVR_DeviceDebugProbe::_cpu_notify_ret() {}
