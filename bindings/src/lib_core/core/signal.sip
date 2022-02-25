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

%ModuleHeaderCode
//#include "core/sim_signal.h"

// #ifdef __cplusplus
// extern "C" {
// #endif
// void fill_sigdata_with_pyobj(signal_data_t* sigdata, PyObject* obj);
// #ifdef __cplusplus
// }
// #endif

//typedef AVR_Signal::Hook AVR_SignalHook;

%End

struct signal_data_t {
%TypeHeaderCode
#include "core/sim_signal.h"
%End

%TypeCode
void fill_sigdata_with_pyobj(signal_data_t* sigdata, PyObject* obj)
{
	 if (PyFloat_Check(obj))
		sigdata->d = PyFloat_AsDouble(obj);
	else if (PyLong_Check(obj)) {
        long long v = PyLong_AsLongLong(obj);
        if (v >= 0)
            sigdata->u = (unsigned long) v;
        else
            sigdata->i = (long) v;
    }
	else if (PyUnicode_Check(obj))
		sigdata->s = PyUnicode_AsUTF8(obj);
	else if (PyBytes_Check(obj))
		sigdata->s = PyBytes_AsString(obj);
	else {
		int iserr = 0;
		const sipTypeDef *sipType = sipTypeFromPyTypeObject((PyTypeObject*)(obj->ob_type));
		if (sipType)
			sigdata->p = sipConvertToType(obj, sipType, Py_None, 0, SIP_NULLPTR, &iserr);
		else
			sigdata->p = obj;
	}
}
%End

	uint16_t sigid;
	uint32_t index;
	
	//void* p;
	double d /NoSetter/;
	uint32_t u /NoSetter/;
	int32_t i /NoSetter/;
	const char* s /NoSetter, Encoding="None"/;
    
    signal_data_t(uint16_t sigid, uint32_t index, SIP_PYOBJECT data);
	%MethodCode
		sipCpp = new signal_data_t();
		sipCpp->sigid = a0;
		sipCpp->index = a1;
		fill_sigdata_with_pyobj(sipCpp, a2);
	%End
    
    void set_data(SIP_PYOBJECT data);
	%MethodCode
		fill_sigdata_with_pyobj(sipCpp, a0);
	%End
    
	SIP_PYOBJECT p(SIP_PYTYPE pytype = SIP_NULLPTR);
	%MethodCode
		if (sipCpp->p == SIP_NULLPTR) {
			Py_INCREF(Py_None);
			sipRes = Py_None;
		}
		else if (a0 == SIP_NULLPTR) {
            sipRes = sipConvertFromVoidPtr(sipCpp->p);
        }
        else{
			const sipTypeDef *sipType = sipTypeFromPyTypeObject((PyTypeObject*)a0);
			if (sipType) {
				sipRes = sipConvertFromType(sipCpp->p, sipType, SIP_NULLPTR);
			} else {
				Py_INCREF(Py_None);
				sipRes = Py_None;
			}
        }
	%End
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

extern void fill_sigdata_with_pyobj(signal_data_t* sigdata, PyObject* obj); 
%End

public:

    AVR_Signal();
    
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
		fill_sigdata_with_pyobj(&sigdata, a2);
		sipCpp->raise(sigdata);
	%End
    
};
