/*
 * types.sip
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

typedef long long         cycle_count_t;
typedef unsigned long     mem_addr_t;
typedef unsigned long     flash_addr_t;
typedef short             int_vect_t;


const cycle_count_t INVALID_CYCLE;


class reg_addr_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

public:

    reg_addr_t(short = -1);

    bool valid() const;

    int __index__() const;
    %MethodCode
        sipRes = (short)(*sipCpp);
    %End

    int __int__() const;
    %MethodCode
        sipRes = (short)(*sipCpp);
    %End

    const char* __repr__() const;
    %MethodCode
        if (sipCpp->valid())
            return PyUnicode_FromFormat("[0x%04x]", (short)(*sipCpp));
        else
            return PyUnicode_FromString("INVALID_REGISTER");
    %End

    bool __eq__(const reg_addr_t&) const;
    %MethodCode
        sipRes = sipCpp->valid() ? ((short)(*sipCpp) == (short)(*a0)) : !a0->valid();
    %End

    bool __ne__(const reg_addr_t&) const;
    %MethodCode
        sipRes = sipCpp->valid() ? ((short)(*sipCpp) != (short)(*a0)) : a0->valid();
    %End

    bool __lt__(const reg_addr_t&) const;
    %MethodCode
        sipRes = sipCpp->valid() ? ((short)(*sipCpp) < (short)(*a0)) : a0->valid();
    %End

    bool __le__(const reg_addr_t&) const;
    %MethodCode
        sipRes = sipCpp->valid() ? ((short)(*sipCpp) <= (short)(*a0)) : true;
    %End

    bool __gt__(const reg_addr_t&) const;
    %MethodCode
        sipRes = sipCpp->valid() ? ((short)(*sipCpp) > (short)(*a0)) : false;
    %End

    bool __ge__(const reg_addr_t&) const;
    %MethodCode
        sipRes = sipCpp->valid() ? ((short)(*sipCpp) >= (short)(*a0)) : !a0->valid();
    %End

    //Ensures a seamless conversion from a Python integer
    %ConvertToTypeCode
        //Type check : we can convert from either a wrapped reg_addr_t or a Python integer
        if (sipIsErr == 0)
            return PyObject_TypeCheck(sipPy, sipTypeAsPyTypeObject(sipType_reg_addr_t)) ||
                   PyLong_Check(sipPy);

        //Conversion from Python integer to unwrapped reg_addr_t
        if (PyLong_Check(sipPy)) {
            *sipCppPtr = new reg_addr_t(PyLong_AsLong(sipPy));
            return sipGetState(sipTransferObj);
        }

        //Unwrapping of a reg_addr_t
        *sipCppPtr = reinterpret_cast<reg_addr_t*>(sipConvertToType(sipPy,
                                                                    sipType_reg_addr_t,
                                                                    sipTransferObj,
                                                                    SIP_NO_CONVERTORS,
                                                                    0,
                                                                    sipIsErr));
        return 0;
    %End
};


const reg_addr_t INVALID_REGISTER;


struct bitmask_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

    uint8_t bit;
    uint8_t mask;

    bitmask_t(uint8_t, uint8_t);
    bitmask_t(uint8_t);
    bitmask_t();
    bitmask_t(const bitmask_t&);
    bitmask_t(const regbit_t&);

    uint8_t extract(uint8_t) const;
    int bitcount() const;

    const char* __repr__() const;
    %MethodCode
        return PyUnicode_FromFormat("Bitmask(%d, %02x)", sipCpp->bit, sipCpp->mask);
    %End

};


struct regbit_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

public:

    reg_addr_t addr;
    uint8_t bit;
    uint8_t mask;

    regbit_t(reg_addr_t, uint8_t, uint8_t);
    regbit_t(reg_addr_t, uint8_t);
    regbit_t(reg_addr_t, const bitmask_t&);
    regbit_t(reg_addr_t);
    regbit_t();
    regbit_t(const regbit_t&);

    bool valid() const;
    uint8_t extract(uint8_t) const;
    uint8_t set_to(uint8_t, uint8_t = 0xFF) const;
    uint8_t clear_from(uint8_t, uint8_t = 0xFF) const;
    uint8_t replace(uint8_t, uint8_t) const;
    int bitcount() const;

    const char* __repr__() const;
    %MethodCode
        if (sipCpp->valid())
            return PyUnicode_FromFormat("Regbit(%04x, %d, %02x)", (short)(sipCpp->addr), sipCpp->bit, sipCpp->mask);
        else
            return PyUnicode_FromString("Regbit(INVALID)");
    %End

};


class regbit_compound_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

public:

    regbit_compound_t();
    regbit_compound_t(const regbit_t&);
    regbit_compound_t(const std::vector<regbit_t>&);
    regbit_compound_t(const regbit_compound_t&);

    void add(const regbit_t&);

    regbit_t __getitem__(int) const;
    %MethodCode
        Py_ssize_t i = sipConvertFromSequenceIndex(a0, sipCpp->size());
        if (i < 0)
            sipIsErr = 1;
        else
            sipRes = new regbit_t((*sipCpp)[a0]);
    %End

    Py_ssize_t __len__() const;
    %MethodCode
        sipRes = sipCpp->size();
    %End

    bool addr_match(reg_addr_t) const;
    uint64_t compound(uint8_t, size_t) const;
    uint8_t extract(uint64_t, size_t) const;

};


typedef unsigned long sim_id_t;
typedef sim_id_t ctl_id_t;

const char* id_to_str(sim_id_t);
%MethodCode
    sipRes = id_to_str(a0).c_str();
%End

sim_id_t str_to_id(const char*);


class vardata_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

%TypeCode
void fill_vardata_with_pyobj(vardata_t* d, PyObject* obj)
{
    if (!obj || obj == Py_None) {
        *d = vardata_t();
    }
    else if (PyFloat_Check(obj)) {
        *d = PyFloat_AsDouble(obj);
    }
    else if (PyLong_Check(obj)) {
        int is_pos = PyObject_RichCompareBool(obj, PyLong_FromLong(0), Py_GE);
        if (is_pos == 1)
            *d = PyLong_AsUnsignedLongLong(obj);
        else
            *d = PyLong_AsLongLong(obj);
    }
    else if (PyUnicode_Check(obj)) {
        *d = PyUnicode_AsUTF8(obj);
    }
    else if (PyBytes_Check(obj)) {
        vardata_t v = vardata_t((uint8_t*) PyBytes_AsString(obj), PyBytes_GET_SIZE(obj));
        *d = v;
    }
    else {
        int iserr = 0;
        const sipTypeDef *sipType = sipTypeFromPyTypeObject((PyTypeObject*)(obj->ob_type));
        if (sipType)
            *d = sipConvertToType(obj, sipType, Py_None, 0, SIP_NULLPTR, &iserr);
        else
            *d = obj;
    }
}

PyObject* vardata_as_pyobject(const vardata_t* v, PyObject* pytype)
{
    if (v->type() != vardata_t::Pointer)
        return nullptr;

    void* p = v->as_ptr();
    PyObject* res;
    if (p == SIP_NULLPTR) {
        Py_INCREF(Py_None);
        res = Py_None;
    }
    else if (pytype == SIP_NULLPTR) {
        res = sipConvertFromVoidPtr(p);
    }
    else{
        const sipTypeDef *sipType = sipTypeFromPyTypeObject((PyTypeObject*) pytype);
        if (sipType) {
            res = sipConvertFromType(p, sipType, SIP_NULLPTR);
        } else {
            Py_INCREF(Py_None);
            res = Py_None;
        }
    }
    return res;
}

%End

public:

    enum Type {
        Invalid,
        Pointer,
        Double,
        Uinteger,
        Integer,
        String,
        Bytes
    };

    vardata_t();
    vardata_t(const vardata_t&);
    vardata_t(SIP_PYOBJECT);
    %MethodCode
        sipCpp = new vardata_t();
        fill_vardata_with_pyobj(sipCpp, a0);
    %End

    vardata_t::Type type() const;

    //void* as_ptr() const;
    SIP_PYOBJECT as_ptr(SIP_PYTYPE = SIP_NULLPTR) const;
    %MethodCode
        sipRes = vardata_as_pyobject(sipCpp, a0);
        if (!sipRes)
            PyErr_SetString(PyExc_ValueError, "Invalid type");
    %End

    const char* as_str() const;
    double as_double() const;
    unsigned long long as_uint() const;
    long long as_int() const;

    //vardata_t& operator=(void* p);
    //vardata_t& operator=(const char* s);
    //vardata_t& operator=(double d);
    //vardata_t& operator=(unsigned long long u);
    //vardata_t& operator=(long long i);
    void set(SIP_PYOBJECT);
    %MethodCode
        fill_vardata_with_pyobj(sipCpp, a0);
    %End

    SIP_PYOBJECT value(SIP_PYTYPE = SIP_NULLPTR) const;
    %MethodCode
    switch(sipCpp->type()) {
        case vardata_t::Invalid:
            PyErr_SetString(PyExc_ValueError, "Invalid vardata");
            sipRes = SIP_NULLPTR;
            break;
        case vardata_t::Pointer:
            sipRes = vardata_as_pyobject(sipCpp, a0);
            break;
        case vardata_t::String:
            sipRes = PyUnicode_FromString(sipCpp->as_str());
            break;
        case vardata_t::Double:
            sipRes = PyFloat_FromDouble(sipCpp->as_double());
            break;
        case vardata_t::Uinteger:
            sipRes = PyLong_FromUnsignedLongLong(sipCpp->as_uint());
            break;
        case vardata_t::Integer:
            sipRes = PyLong_FromLongLong(sipCpp->as_int());
            break;
        case vardata_t::Bytes:
            sipRes = PyBytes_FromStringAndSize((const char*) sipCpp->as_bytes(), sipCpp->size());
            break;
    }
    %End

    bool __eq__(SIP_PYOBJECT) const;
    %MethodCode
        vardata_t v;
        fill_vardata_with_pyobj(&v, a0);
        sipRes = (*sipCpp == v);
    %End

};
