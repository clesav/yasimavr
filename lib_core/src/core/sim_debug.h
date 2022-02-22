/*
 * sim_debug.h
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

#ifndef __YASIMAVR_DEBUG_H__
#define __YASIMAVR_DEBUG_H__

#include "sim_types.h"
#include "sim_device.h"

//=======================================================================================
/*
 * AVR_DeviceDebugProbe is a facility to access the inner state of a AVR_Device and its core
 * to debug the execution of a firmware.
 * It provides:
 *  - function to read/write directly into CPU registers, I/O registers, flash, data space
 *  - soft breakpoints
 *  - data watchpoints
 *  - device reset and state change
 */
class DLL_EXPORT AVR_DeviceDebugProbe {
    
public:

	enum WatchpointKind {
		Watchpoint_Write = 0x01,
		Watchpoint_Read = 0x02,
		Watchpoint_Access = 0x04
	};

	AVR_DeviceDebugProbe();
    ~AVR_DeviceDebugProbe();
    
    AVR_Device* device() const;

    void attach(AVR_Device& device);
    void detach();
    bool attached() const;

    void reset_device();
    void set_device_state(AVR_Device::State state);

    //Access to general purpose CPU registers
    void write_gpreg(unsigned int num, uint8_t value);
    uint8_t read_gpreg(unsigned int num);

    //Access to the SREG register
    void write_sreg(uint8_t value);
    uint8_t read_sreg();

    //Access to the Stack Pointer
    void write_sp(uint16_t value);
    uint16_t read_sp();

    //Access to the Program Counter
    void write_pc(uint32_t value);
    uint32_t read_pc();

    //Access to I/O registers. From the peripherals point of view, it's
    //the same as a CPU access
    void write_ioreg(reg_addr_t addr, uint8_t value);
    uint8_t read_ioreg(reg_addr_t addr);

    //Access to the flash memory
    void write_flash(flash_addr_t addr, uint8_t* buf, uint32_t len);
    uint32_t read_flash(flash_addr_t addr, uint8_t* buf, uint32_t len);

    //Access to the data space
    void write_data(mem_addr_t addr, uint8_t* buf, uint32_t len);
    void read_data(mem_addr_t addr, uint8_t* buf, uint32_t len);

    //Breakpoint management
    void insert_breakpoint(flash_addr_t addr);
    void remove_breakpoint(flash_addr_t addr);

    //Watchpoint management
    void insert_watchpoint(mem_addr_t addr, mem_addr_t len, WatchpointKind kind);
    void remove_watchpoint(mem_addr_t addr, WatchpointKind kind);

    //Callbacks from the CPU for notifications
    void _cpu_notify_data_read(mem_addr_t addr);
    void _cpu_notify_data_write(mem_addr_t addr);
    void _cpu_notify_jump(flash_addr_t addr);
    void _cpu_notify_call(flash_addr_t addr);
    void _cpu_notify_ret();

private:

    struct watchpoint_t {
    	mem_addr_t addr;
    	mem_addr_t len;
    	uint8_t kind;
    };

    AVR_Device* m_device;
    //Mapping containers PC => breakpoint
    std::map<flash_addr_t, breakpoint_t*> m_breakpoints;
    //Mapping containers mem address => watchpoint
    std::map<mem_addr_t, watchpoint_t*> m_watchpoints;

};

inline AVR_Device* AVR_DeviceDebugProbe::device() const
{
	return m_device;
}

inline bool AVR_DeviceDebugProbe::attached() const
{
	return !!m_device;
}

#endif //__YASIMAVR_DEBUG_H__
