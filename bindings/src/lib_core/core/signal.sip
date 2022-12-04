/*
 * signal.sip
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
    AVR_Signal(const AVR_Signal&);
    virtual ~AVR_Signal();

    void connect_hook(AVR_SignalHook*, uint16_t = 0);
    void disconnect_hook(AVR_SignalHook*);

    void raise() /PyName=raise_/;
    virtual void raise(const signal_data_t&) /PyName=raise_/;

    void raise(uint16_t, SIP_PYOBJECT, uint32_t = 0) /PyName=raise_/;
    %MethodCode
        signal_data_t sigdata;
        sigdata.sigid = a0;
        fill_vardata_with_pyobj(&sigdata.data, a1);
        sigdata.index = a2;
        sipCpp->raise(sigdata);
    %End

};


class AVR_DataSignal : public AVR_Signal {
%TypeHeaderCode
#include "core/sim_signal.h"

extern void fill_vardata_with_pyobj(vardata_t* d, PyObject* obj);
%End

public:

    vardata_t data(uint16_t, uint32_t = 0) const;
    bool has_data(uint16_t, uint32_t = 0) const;
    void set_data(uint16_t, vardata_t, uint32_t = 0);

    void clear();

    virtual void raise(const signal_data_t&);
    void raise(uint16_t, SIP_PYOBJECT, uint32_t = 0);
    %MethodCode
        signal_data_t sigdata;
        sigdata.sigid = a0;
        fill_vardata_with_pyobj(&sigdata.data, a1);
        sigdata.index = a2;
        sipCpp->raise(sigdata);
    %End

};


class AVR_CallableSignalHook : public AVR_SignalHook {
%TypeHeaderCode
#include "core/sim_signal.h"
%End

%TypeCode
class AVR_CallableSignalHook : public AVR_SignalHook {

public:

    AVR_CallableSignalHook(PyObject* callback)
    :m_callback(callback)
    {
        Py_INCREF(m_callback);
    }

    AVR_CallableSignalHook(const AVR_CallableSignalHook& other)
    :AVR_CallableSignalHook(other.m_callback)
    {}

    virtual ~AVR_CallableSignalHook()
    {
        Py_DECREF(m_callback);
    }

    virtual void raised(const signal_data_t& sigdata, uint16_t hooktag) override final
    {
        //Ensure the GIL is held for the callback
        PyGILState_STATE pystate = PyGILState_Ensure();
        //Wrap the sigdata argument into a Python object using the SIP API
        signal_data_t s = sigdata;
        PyObject* pysigdata = sipConvertFromType(&s, sipType_signal_data_t, SIP_NULLPTR);
        //Call the callback
        PyObject* res = PyObject_CallFunction(m_callback, "OH", pysigdata, hooktag);
        //Reference cleanup
        Py_XDECREF(res);
        Py_DECREF(pysigdata);
        //Release the GIL
        PyGILState_Release(pystate);
    }

    AVR_CallableSignalHook& operator=(const AVR_CallableSignalHook& other)
    {
        Py_DECREF(m_callback);
        m_callback = other.m_callback;
        Py_INCREF(m_callback);
        return *this;
    }

private:

    PyObject* m_callback;

};
%End

public:

    AVR_CallableSignalHook(SIP_PYCALLABLE);
    AVR_CallableSignalHook(const AVR_CallableSignalHook&);
    virtual ~AVR_CallableSignalHook();
    virtual void raised(const signal_data_t&, uint16_t) final;

};


class AVR_DataSignalMux : public AVR_SignalHook {
%TypeHeaderCode
#include "core/sim_signal.h"
%End

public:

    AVR_DataSignalMux();

    virtual void raised(const signal_data_t&, uint16_t);

    size_t add_mux();
    size_t add_mux(AVR_DataSignal&);
    size_t add_mux(AVR_DataSignal&, uint16_t);
    size_t add_mux(AVR_DataSignal&, uint16_t, uint32_t);

    AVR_DataSignal& signal();

    void set_selection(size_t);
    size_t selected_index() const;
    bool connected() const;

};
