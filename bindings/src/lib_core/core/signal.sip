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

    int sigid;
    long long index;
    vardata_t data;

};


class SignalHook {
%TypeHeaderCode
#include "core/sim_signal.h"
%End

public:

    virtual void raised(const signal_data_t&, int) = 0;

};


class Signal {
%TypeHeaderCode
#include "core/sim_signal.h"
%End

%TypeCode
extern void fill_vardata_with_pyobj(vardata_t* d, PyObject* obj);
%End

public:

    Signal();
    Signal(const Signal&);

    void connect(SignalHook&, int = 0);
    void disconnect(SignalHook&);

    virtual void raise(const signal_data_t&) /PyName=raise_/;

    void raise(int = 0, SIP_PYOBJECT = nullptr, long long = 0) /PyName=raise_/;
    %MethodCode
        signal_data_t sigdata;
        sigdata.sigid = a0;
        fill_vardata_with_pyobj(&sigdata.data, a1);
        sigdata.index = a2;
        sipCpp->raise(sigdata);
    %End

};


class DataSignal : public Signal {
%TypeHeaderCode
#include "core/sim_signal.h"
%End

%TypeCode
extern void fill_vardata_with_pyobj(vardata_t* d, PyObject* obj);
%End

public:

    vardata_t data(int, long long = 0) const;
    bool has_data(int, long long = 0) const;
    void set_data(int, SIP_PYOBJECT = nullptr, long long = 0);
    %MethodCode
        vardata_t v;
        fill_vardata_with_pyobj(&v, a1);
        sipCpp->set_data(a0, v, a2);
    %End

    void clear();

    //The PyName annotation doesn't seem required for an override
    virtual void raise(const signal_data_t&);

    //Duplicate from Signal, because it's not virtual
    void raise(int = 0, SIP_PYOBJECT = nullptr, long long = 0);
    %MethodCode
        signal_data_t sigdata;
        sigdata.sigid = a0;
        fill_vardata_with_pyobj(&sigdata.data, a1);
        sigdata.index = a2;
        sipCpp->raise(sigdata);
    %End

};


class CallableSignalHook : public SignalHook {
%TypeHeaderCode
#include "core/sim_signal.h"
%End

%TypeCode
class CallableSignalHook : public SignalHook {

public:

    CallableSignalHook(PyObject* callback)
    :m_callback(callback)
    {
        Py_INCREF(m_callback);
    }

    CallableSignalHook(const CallableSignalHook& other)
    :CallableSignalHook(other.m_callback)
    {}

    virtual ~CallableSignalHook()
    {
        Py_DECREF(m_callback);
    }

    virtual void raised(const signal_data_t& sigdata, int hooktag) override final
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

    CallableSignalHook& operator=(const CallableSignalHook& other)
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

    CallableSignalHook(SIP_PYCALLABLE);
    CallableSignalHook(const CallableSignalHook&);

    virtual void raised(const signal_data_t&, int) final;

};


class DataSignalMux : public SignalHook {
%TypeHeaderCode
#include "core/sim_signal.h"
%End

public:

    DataSignalMux();

    virtual void raised(const signal_data_t&, int);

    size_t add_mux();
    size_t add_mux(DataSignal&);
    size_t add_mux(DataSignal&, int);
    size_t add_mux(DataSignal&, int, long long);

    DataSignal& signal();

    void set_selection(size_t);
    size_t selected_index() const;
    bool connected() const;

};
