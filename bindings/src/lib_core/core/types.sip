/*
 * types.sip
 *
 *  Copyright 2022-2026 Clement Savergne <csavergne@yahoo.com>

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

%UnitCode
#define _PYTYPE_CHECK(obj, _TYPE_) \
    PyObject_TypeCheck(obj, sipTypeAsPyTypeObject(sipType_ ## _TYPE_))
%End


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
    } else {
        //sipPy is a wrapped reg_addr_t, make a C++ copy of it
        reg_addr_t* wrappedCpp = (reg_addr_t*) sipGetAddress((sipSimpleWrapper*) sipPy);
        *sipCppPtr = new reg_addr_t(*wrappedCpp);
    }

    return sipGetState(sipTransferObj);
%End

};


const reg_addr_t INVALID_REGISTER;


//=======================================================================================

struct bitmask_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

    uint8_t mask;

    bitmask_t(uint8_t = 0);

    uint8_t replace(uint8_t, uint8_t) const;
    uint8_t set_from(uint8_t) const;
    uint8_t clear_from(uint8_t) const;
    bitmask_t __or__(const bitmask_t&) const;
    bitmask_t __and__(const bitmask_t&) const;
    bitmask_t __xor__(const bitmask_t&) const;
    bitmask_t __invert__() const;
    void __ior__(const bitmask_t&);
    void __iand__(const bitmask_t&);
    void __ixor__(const bitmask_t&);
    uint8_t __and__(uint8_t) const;
    unsigned int bitcount() const;
    bool bit(unsigned int) const;

//Ensures a seamless conversion from a bitmask_t, bitspec_t or regbit_t object
%ConvertToTypeCode
    if (sipIsErr == 0)
        return _PYTYPE_CHECK(sipPy, bitmask_t) ||
               _PYTYPE_CHECK(sipPy, bitspec_t) ||
               _PYTYPE_CHECK(sipPy, regbit_t);

    if (_PYTYPE_CHECK(sipPy, bitmask_t)) {
        bitmask_t* wrappedCpp = (bitmask_t*) sipGetAddress((sipSimpleWrapper*) sipPy);
        *sipCppPtr = new bitmask_t(*wrappedCpp);
    }
    else if (_PYTYPE_CHECK(sipPy, bitspec_t)) {
        bitspec_t* wrappedCpp = (bitspec_t*) sipGetAddress((sipSimpleWrapper*) sipPy);
        *sipCppPtr = new bitmask_t((bitmask_t)(*wrappedCpp));
    }
    else if (_PYTYPE_CHECK(sipPy, regbit_t)) {
        regbit_t* wrappedCpp = (regbit_t*) sipGetAddress((sipSimpleWrapper*) sipPy);
        *sipCppPtr = new bitmask_t((bitmask_t)(*wrappedCpp));
    }

    return sipGetState(sipTransferObj);
%End

};


//=======================================================================================

struct regmask_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

    reg_addr_t addr;
    bitmask_t mask;

    regmask_t();
    %MethodCode
        sipCpp = new regmask_t();
    %End

    regmask_t(reg_addr_t, bitmask_t);
    %MethodCode
        sipCpp = new regmask_t({ *a0, *a1 });
    %End

    regmask_t __or__(const regmask_t&) const;
    bool __eq__(reg_addr_t) const;
    bool __ne__(reg_addr_t) const;

//Ensures a seamless conversion from a regbit_t or regmask_t object
%ConvertToTypeCode
    if (sipIsErr == 0)
        return _PYTYPE_CHECK(sipPy, regmask_t) ||
               _PYTYPE_CHECK(sipPy, regbit_t);

    if (_PYTYPE_CHECK(sipPy, regmask_t)) {
        regmask_t* wrappedCpp = (regmask_t*) sipGetAddress((sipSimpleWrapper*) sipPy);
        *sipCppPtr = new regmask_t(*wrappedCpp);
    }
    else if (_PYTYPE_CHECK(sipPy, regbit_t)) {
        regbit_t* wrappedCpp = (regbit_t*) sipGetAddress((sipSimpleWrapper*) sipPy);
        *sipCppPtr = new regmask_t(*wrappedCpp);
    }

    return sipGetState(sipTransferObj);
%End

};


//=======================================================================================

class bitspec_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

public:

    bitspec_t();
    bitspec_t(uint8_t, uint8_t);
    bitspec_t(uint8_t);

    bool valid() const;
    uint8_t shift_and_mask(uint8_t) const;
    uint8_t extract(uint8_t) const;
    uint8_t set_from(uint8_t) const;
    uint8_t clear_from(uint8_t) const;
    uint8_t replace(uint8_t, uint8_t) const;
    unsigned int bitcount() const;
    bitspec_t bit(uint8_t) const;
    uint8_t lsb() const;
    uint8_t msb() const;
    uint8_t __and__(uint8_t) const;
    bitmask_t __or__(const bitspec_t&) const;

    const char* __repr__() const;
    %MethodCode
        if (!sipCpp->valid())
            return PyUnicode_FromString("bitspec_t()");

        uint8_t lsb = sipCpp->lsb();
        uint8_t msb = sipCpp->msb();
        if (msb == lsb)
            return PyUnicode_FromFormat("bitspec_t(%u)", lsb);
        else
            return PyUnicode_FromFormat("bitspec_t(%u-%u)", lsb, msb);
    %End

//Ensures a seamless conversion from a regbit_t or bitspec_t object
%ConvertToTypeCode
    if (sipIsErr == 0)
        return _PYTYPE_CHECK(sipPy, bitspec_t) ||
               _PYTYPE_CHECK(sipPy, regbit_t);

    if (_PYTYPE_CHECK(sipPy, bitspec_t)) {
        bitspec_t* wrappedCpp = (bitspec_t*) sipGetAddress((sipSimpleWrapper*) sipPy);
        *sipCppPtr = new bitspec_t(*wrappedCpp);
    }
    else if (_PYTYPE_CHECK(sipPy, regbit_t)) {
        regbit_t* wrappedCpp = (regbit_t*) sipGetAddress((sipSimpleWrapper*) sipPy);
        *sipCppPtr = new bitspec_t(*wrappedCpp);
    }

    return sipGetState(sipTransferObj);
%End

};


//=======================================================================================

class regbit_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

%TypeCode
void regbit_to_str(char* buf, const regbit_t& rb)
{
    if (!rb.valid()) {
        sprintf(buf, "regbit_t()");
    } else {
        uint8_t lsb = rb.lsb();
        uint8_t msb = rb.msb();
        if (msb == lsb)
            sprintf(buf, "regbit_t(0x%04x:%hhu)", (short)(rb.addr), lsb);
        else
            sprintf(buf, "regbit_t(0x%04x:%hhu-%hhu)", (short)(rb.addr), lsb, msb);
    }
}
%End

public:

    reg_addr_t addr;

    regbit_t();
    regbit_t(reg_addr_t);
    regbit_t(reg_addr_t, uint8_t, uint8_t);
    regbit_t(reg_addr_t, uint8_t);
    regbit_t(reg_addr_t, const bitspec_t&);

    bool valid() const;
    uint8_t shift_and_mask(uint8_t) const;
    uint8_t extract(uint8_t) const;
    uint8_t set_from(uint8_t) const;
    uint8_t clear_from(uint8_t) const;
    uint8_t replace(uint8_t, uint8_t) const;
    unsigned int bitcount() const;
    uint8_t lsb() const;
    uint8_t msb() const;
    regbit_t bit(uint8_t) const;
    bool __eq__(reg_addr_t) const;
    bool __ne__(reg_addr_t) const;
    uint8_t __and__(uint8_t) const;

    const char* __repr__() const;
    %MethodCode
        char buf[50];
        regbit_to_str(buf, *sipCpp);
        return PyUnicode_FromString(buf);
    %End

};


//=======================================================================================

class regbit_compound_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

%TypeCode
extern void regbit_to_str(char* buf, const regbit_t& rb);
%End

public:

    regbit_compound_t();
    regbit_compound_t(const regbit_t&);
    regbit_compound_t(const std::vector<regbit_t>&);

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

    unsigned int bitcount() const;

    const char* __repr__() const;
    %MethodCode
        std::string s = "regbit_compound_t(";
        bool use_sep = false;
        for (const regbit_t& rb : *sipCpp) {
            if (use_sep)
                s += ",";
            use_sep = true;

            char buf_rbs[50];
            regbit_to_str(buf_rbs, rb);
            s += buf_rbs;
        }
        s += ")";

        return PyUnicode_FromString(s.c_str());
    %End

};


//=======================================================================================

class vardata_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

%TypeCode

void fill_vardata_with_pyobj(vardata_t* d, PyObject* obj, PyObject** refObj)
{
    if (refObj != NULL)
        *refObj = NULL;

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
    else if (PyObject_CheckBuffer(obj)) {
        if (refObj == NULL) {
            PyErr_SetString(PyExc_Exception, "Bytes type for vardata_t not supported here.");
        } else {
            PyObject* py_view = PyMemoryView_GetContiguous(obj, PyBUF_READ, 'C');
            Py_buffer* py_buf = PyMemoryView_GET_BUFFER(py_view);
            *d = bytes_view_t({ (uint8_t*) py_buf->buf, (size_t) py_buf->len });
            *refObj = py_view;
        }
    }
    else {
        const sipTypeDef *sipType = sipTypeFromPyTypeObject((PyTypeObject*)(obj->ob_type));
        if (sipType)
            *d = sipGetAddress((sipSimpleWrapper*) obj);
        else
            *d = obj;
    }
}

vardata_t pyobject_as_vardata(PyObject* obj, PyObject** refObj)
{
    if (PyObject_TypeCheck(obj, sipTypeAsPyTypeObject(sipType_vardata_t))) {
        if (refObj != NULL)
            *refObj = NULL;
        return *(vardata_t*) sipGetAddress((sipSimpleWrapper*) obj);
    } else {
        vardata_t v;
        fill_vardata_with_pyobj(&v, obj, refObj);
        return v;
    }
}

PyObject* vardata_as_pyobject(const vardata_t* v, PyObject* pytype)
{
    if (v->type() != vardata_t::Pointer)
        return nullptr;

    void* p = v->as_ptr<void>();
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
        Bytes,
    };

    vardata_t();
    vardata_t(SIP_PYOBJECT);
    %MethodCode
        sipCpp = new vardata_t();
        PyObject* refObj;
        fill_vardata_with_pyobj(sipCpp, a0, &refObj);
        if (refObj)
            sipSetUserObject(sipSelf, refObj);
    %End

    vardata_t::Type type() const;

    SIP_PYOBJECT as_ptr(SIP_PYTYPE = SIP_NULLPTR) const;
    %MethodCode
        sipRes = vardata_as_pyobject(sipCpp, a0);
        if (!sipRes)
            PyErr_SetString(PyExc_ValueError, "Invalid type");
    %End

    double as_double() const;
    unsigned long long as_uint() const;
    long long as_int() const;
    SIP_PYOBJECT as_bytes() const /TypeHint="memoryview"/;
    %MethodCode
        sipRes = bytesview_to_pyview(sipCpp->as_bytes());
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
            sipRes = bytesview_to_pyview(sipCpp->as_bytes());
            break;
    }
    %End

    bool __eq__(SIP_PYOBJECT) const;
    %MethodCode
        PyObject* refObj;
        vardata_t v = pyobject_as_vardata(a0, &refObj);
        sipRes = (*sipCpp == v);
        Py_XDECREF(refObj);
    %End

    bool __ne__(SIP_PYOBJECT) const;
    %MethodCode
        PyObject* refObj;
        vardata_t v = pyobject_as_vardata(a0, &refObj);
        sipRes = (*sipCpp != v);
        Py_XDECREF(refObj);
    %End

};


//=======================================================================================

class sim_id_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

public:

    sim_id_t();
    sim_id_t(uint64_t);
    sim_id_t(const char*);
    sim_id_t(const sim_id_t&);

    std::string str() const;

    const char* __repr__() const;
    %MethodCode
        if (*sipCpp)
            return PyUnicode_FromFormat("[%s]", sipCpp->str().c_str());
        else
            return PyUnicode_FromString("INVALID_ID");
    %End

    bool __eq__(const sim_id_t&) const;
    sim_id_t __add__(char c) const;
    sim_id_t __iadd__(char c);

//Ensures a seamless conversion from a Python integer or a string
%ConvertToTypeCode
    //Type check : we can convert from either a wrapped sim_id_t, a string or an integer
    if (sipIsErr == NULL)
        return PyObject_TypeCheck(sipPy, sipTypeAsPyTypeObject(sipType_sim_id_t)) ||
               PyLong_Check(sipPy) ||
               PyUnicode_Check(sipPy);

    //Conversion from Python integer
    if (PyLong_Check(sipPy)) {
        *sipCppPtr = new sim_id_t(PyLong_AsUnsignedLongLong(sipPy));
    }

    //Conversion from a string, that is encoded to ASCII first
    else if (PyUnicode_Check(sipPy)) {
        PyObject* py_ascii = PyUnicode_AsASCIIString(sipPy);
        if (!py_ascii) {
            *sipIsErr = 1;
        } else {
            char* s = PyBytes_AsString(py_ascii);
            *sipCppPtr = new sim_id_t(s);
            Py_DECREF(py_ascii);
        }
    }

    else {
        //Wrapped sim_id_t, make a C++ copy of it
        sim_id_t* wrappedObj = (sim_id_t*) sipGetAddress((sipSimpleWrapper*) sipPy);
        *sipCppPtr = new sim_id_t(*wrappedObj);
    }

    return sipGetState(sipTransferObj);
%End

};

typedef sim_id_t ctl_id_t;

