/*
 * debug.sip
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

class AVR_DeviceDebugProbe {
%TypeHeaderCode
#include "core/sim_debug.h"
#include "utils/buffer_utils.h"
%End

public:

	enum WatchpointKind {
		Watchpoint_Write,
		Watchpoint_Read,
		Watchpoint_Access
	};
	
    AVR_DeviceDebugProbe();
     ~AVR_DeviceDebugProbe();
     
    AVR_Device* device() const;
    
    void attach(AVR_Device& /TransferThis/);
    void detach();
    %MethodCode
    	sipCpp->detach();
    	sipTransferBack(sipSelf);
    %End
    bool attached() const;
    
    void reset_device();
    void set_device_state(AVR_Device::State);

    void write_gpreg(unsigned int, uint8_t);
    uint8_t read_gpreg(unsigned int);

    void write_sreg(uint8_t);
    uint8_t read_sreg();

    void write_sp(uint16_t);
    uint16_t read_sp();

    void write_pc(uint32_t);
    uint32_t read_pc();
    
    void write_ioreg(reg_addr_t, uint8_t);
    uint8_t read_ioreg(reg_addr_t);
    
    void write_flash(flash_addr_t, SIP_PYBUFFER);
    %MethodCode
    	Py_buffer pybuf;
    	PyObject_GetBuffer(a1, &pybuf, PyBUF_SIMPLE);
    	sipCpp->write_flash(a0, (uint8_t*)(pybuf.buf), pybuf.len);
    	PyBuffer_Release(&pybuf);
    %End
    
    SIP_PYBUFFER read_flash(flash_addr_t addr, uint32_t len);
    %MethodCode
    	if (a1) {
    		uint8_t* buf = (uint8_t*) sipMalloc(a1);
    		uint32_t rlen = sipCpp->read_flash(a0, buf, a1);
    		PyObject* pybuf = export_to_pybuffer(sipAPI_core, buf, rlen);
    		sipRes = PyBytes_FromObject(pybuf);
    		sipFree(buf);
    		Py_DECREF(pybuf);
    	} else {
    		sipRes = PyBytes_FromString("");
    	} 
    %End

    void write_data(mem_addr_t addr, SIP_PYBUFFER buf);
    %MethodCode
    	Py_buffer pybuf;
    	PyObject_GetBuffer(a1, &pybuf, PyBUF_SIMPLE);
    	sipCpp->write_data(a0, (uint8_t*)(pybuf.buf), pybuf.len);
    	PyBuffer_Release(&pybuf);
    %End
    
    SIP_PYBUFFER read_data(mem_addr_t addr, uint32_t len);
    %MethodCode
    	if (a1) {
    		uint8_t* buf = (uint8_t*) sipMalloc(a1);
    		sipCpp->read_data(a0, buf, a1);
    		PyObject* pybuf = export_to_pybuffer(sipAPI_core, buf, a1);
    		sipRes = PyBytes_FromObject(pybuf);
    		sipFree(buf);
    		Py_DECREF(pybuf);
    	} else {
    		sipRes = PyBytes_FromString("");
    	} 
    %End
    
    void insert_breakpoint(flash_addr_t);
    void remove_breakpoint(flash_addr_t);

    void insert_watchpoint(mem_addr_t, mem_addr_t, AVR_DeviceDebugProbe::WatchpointKind);
    void remove_watchpoint(mem_addr_t, AVR_DeviceDebugProbe::WatchpointKind);
    
};
