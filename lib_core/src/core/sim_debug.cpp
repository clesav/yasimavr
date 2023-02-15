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
{}

AVR_DeviceDebugProbe::~AVR_DeviceDebugProbe()
{
    if (m_device)
        detach();
}

void AVR_DeviceDebugProbe::attach(AVR_Device& device)
{
    if (m_device) return;

    m_device = &device;
    device.core().dbg_set_debug_probe(this);
}

void AVR_DeviceDebugProbe::detach()
{
    if (!m_device) return;

    //Destroys all the breakpoints
    for (auto it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it) {
        breakpoint_t* bp = it->second;
        m_device->core().dbg_remove_breakpoint(*bp);
        delete bp;
    }
    m_breakpoints.clear();

    //Destroys all the watchpoints
    for (auto it = m_watchpoints.begin(); it != m_watchpoints.end(); ++it) {
        watchpoint_t* wp = it->second;
        delete wp;
    }
    m_watchpoints.clear();

    m_device->core().dbg_set_debug_probe(nullptr);
    m_device = nullptr;
}

void AVR_DeviceDebugProbe::reset_device() const
{
    m_device->reset();
}

void AVR_DeviceDebugProbe::set_device_state(AVR_Device::State state) const
{
    m_device->set_state(state);
}

void AVR_DeviceDebugProbe::write_gpreg(unsigned int num, uint8_t value) const
{
    m_device->core().m_regs[num] = value;
}

uint8_t AVR_DeviceDebugProbe::read_gpreg(unsigned int num) const
{
    return m_device->core().m_regs[num];
}

void AVR_DeviceDebugProbe::write_sreg(uint8_t value) const
{
    m_device->core().write_sreg(value);
}

uint8_t AVR_DeviceDebugProbe::read_sreg() const
{
    return m_device->core().read_sreg();
}

void AVR_DeviceDebugProbe::write_sp(uint16_t value) const
{
    m_device->core().write_sp(value);
}

uint16_t AVR_DeviceDebugProbe::read_sp() const
{
    return m_device->core().read_sp();
}

void AVR_DeviceDebugProbe::write_pc(uint32_t value) const
{
    m_device->core().m_pc = value;
}

uint32_t AVR_DeviceDebugProbe::read_pc() const
{
    return m_device->core().m_pc;
}

void AVR_DeviceDebugProbe::write_ioreg(reg_addr_t addr, uint8_t value) const
{
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

void AVR_DeviceDebugProbe::write_flash(flash_addr_t addr, const uint8_t* buf, uint32_t len) const
{
    AVR_Core& core = m_device->core();

    ADJUST_ADDR_LEN(addr, len, core.config().flashend);

    core.m_flash.dbg_write(buf, addr, len);
}

uint32_t AVR_DeviceDebugProbe::read_flash(flash_addr_t addr, uint8_t* buf, uint32_t len) const
{
    AVR_Core& core = m_device->core();

    ADJUST_ADDR_LEN(addr, len, core.config().flashend);

    core.m_flash.dbg_read(buf, addr, len);

    return len;
}

void AVR_DeviceDebugProbe::write_data(mem_addr_t addr, const uint8_t* buf, uint32_t len) const
{
    //We need to momentarily disable the crash triggered by bad I/O access
    //as we copy the I/O registers as a block
    bool badioopt = m_device->test_option(AVR_Device::Option_IgnoreBadCpuIO);
    m_device->set_option(AVR_Device::Option_IgnoreBadCpuIO, true);

    m_device->core().dbg_write_data(addr, buf, len);

    m_device->set_option(AVR_Device::Option_IgnoreBadCpuIO, badioopt);
}

void AVR_DeviceDebugProbe::read_data(mem_addr_t addr, uint8_t* buf, uint32_t len) const
{
    //Same as write_data()
    bool badioopt = m_device->test_option(AVR_Device::Option_IgnoreBadCpuIO);
    m_device->set_option(AVR_Device::Option_IgnoreBadCpuIO, true);

    m_device->core().dbg_read_data(addr, buf, len);

    m_device->set_option(AVR_Device::Option_IgnoreBadCpuIO, badioopt);
}

void AVR_DeviceDebugProbe::insert_breakpoint(flash_addr_t addr)
{
    if (m_breakpoints.find(addr) != m_breakpoints.end()) return;

    breakpoint_t* bp = new breakpoint_t;
    bp->addr = addr;
    m_device->core().dbg_insert_breakpoint(*bp);
    m_breakpoints[addr] = bp;
}

void AVR_DeviceDebugProbe::remove_breakpoint(flash_addr_t addr)
{
    auto search = m_breakpoints.find(addr);
    if (search == m_breakpoints.end()) return;
    breakpoint_t* bp = search->second;

    m_breakpoints.erase(search);
    m_device->core().dbg_remove_breakpoint(*bp);

    delete bp;
}

void AVR_DeviceDebugProbe::insert_watchpoint(mem_addr_t addr, mem_addr_t len, int flags)
{
    watchpoint_t* wp;
    auto search = m_watchpoints.find(addr);
    if (search != m_watchpoints.end()) {
        //If the watchpoint is found, just OR the flags
        wp = search->second;
        wp->flags |= flags;
    } else {
        wp = new watchpoint_t;
        wp->addr = addr;
        wp->len = len;
        wp->flags = flags;
        m_watchpoints[addr] = wp;
    }
}

void AVR_DeviceDebugProbe::remove_watchpoint(mem_addr_t addr, int flags)
{
    auto search = m_watchpoints.find(addr);
    if (search != m_watchpoints.end()) {
        //If the watchpoint exists, clear its flags
        watchpoint_t* wp = search->second;
        wp->flags &= ~flags;
        //If neither Read or Write event flags are left, destroy the watchpoint
        if (!(wp->flags & (Watchpoint_Write | Watchpoint_Read))) {
            m_watchpoints.erase(search);
            delete wp;
        }
    }
}

void AVR_DeviceDebugProbe::notify_watchpoint(watchpoint_t* wp, int event, mem_addr_t addr)
{
    if (wp->flags & Watchpoint_Break) {
        m_device->logger().wng("Device break on watchpoint A=%04x", addr);
        m_device->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_BREAK, nullptr);
    }

    if (wp->flags & Watchpoint_Signal)
        m_wp_signal.raise_u(event, wp->addr);
}

//Notification when the CPU reads from the RAM. Check if there's a watchpoint associated with the address.
void AVR_DeviceDebugProbe::_cpu_notify_data_read(mem_addr_t addr)
{
    for (auto it = m_watchpoints.begin(); it != m_watchpoints.end(); ++it) {
        watchpoint_t* wp = it->second;
        if (addr >= wp->addr && addr < (wp->addr + wp->len) && (wp->flags & Watchpoint_Read)) {
            notify_watchpoint(wp, Watchpoint_Read, addr);
            return;
        }
    }
}

//Notification when the CPU writes into the RAM. Check if there's a watchpoint associated with the address.
void AVR_DeviceDebugProbe::_cpu_notify_data_write(mem_addr_t addr)
{
    for (auto it = m_watchpoints.begin(); it != m_watchpoints.end(); ++it) {
        watchpoint_t* wp = it->second;
        if (addr >= wp->addr && addr < (wp->addr + wp->len) && (wp->flags & Watchpoint_Write)) {
            notify_watchpoint(wp, Watchpoint_Write, addr);
            return;
        }
    }
}

//For future use maybe
void AVR_DeviceDebugProbe::_cpu_notify_jump(flash_addr_t addr) {}
void AVR_DeviceDebugProbe::_cpu_notify_call(flash_addr_t addr) {}
void AVR_DeviceDebugProbe::_cpu_notify_ret() {}
