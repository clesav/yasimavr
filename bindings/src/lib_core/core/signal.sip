/*
 * signal.sip
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

struct signal_data_t {
%TypeHeaderCode
#include "core/sim_signal.h"
%End

	uint16_t sigid;
	uint32_t index;
	vardata_t data;

};


class AVR_SignalHook {
%TypeHeaderCode
#include "core/sim_signal.h"
%End

public:

	virtual ~AVR_SignalHook();	
	virtual void raised(const signal_data_t&, uint16_t) = 0;
		
};


class AVR_Signal {
%TypeHeaderCode
#include "core/sim_signal.h"

extern void fill_vardata_with_pyobj(vardata_t* d, PyObject* obj); 
%End

public:

    AVR_Signal();
	~AVR_Signal();
    
    void connect_hook(AVR_SignalHook*, uint16_t = 0);
    void disconnect_hook(AVR_SignalHook*);
    
    const signal_data_t& data() const;
    
    void raise() /PyName=raise_/;
    void raise(const signal_data_t&) /PyName=raise_/;
    
	void raise(uint16_t, uint32_t, SIP_PYOBJECT) /PyName=raise_/;
    %MethodCode
		signal_data_t sigdata;
		sigdata.sigid = a0;
		sigdata.index = a1;
		fill_vardata_with_pyobj(&sigdata.data, a2);
		sipCpp->raise(sigdata);
	%End
    
};
