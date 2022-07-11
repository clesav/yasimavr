/*
 * types.sip
 *
 *	Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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


typedef int64_t			cycle_count_t;
typedef uint16_t		reg_addr_t;
typedef uint32_t    	mem_addr_t;
typedef uint32_t    	flash_addr_t;
typedef int				int_vect_t;


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
	regbit_t(reg_addr_t);
	regbit_t();
	regbit_t(reg_addr_t, const bitmask_t&);
	
	bool valid() const;
	uint8_t extract(uint8_t) const;
	int bitcount() const;
	
	const char* __repr__() const;
	%MethodCode
		return PyUnicode_FromFormat("Regbit(%04x, %d, %02x)", sipCpp->addr, sipCpp->bit, sipCpp->mask);
	%End
	
};


const char* id_to_str(uint32_t);
%MethodCode
	sipRes = id_to_str(a0).c_str();
%End

uint32_t str_to_id(const char*);


class vardata_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

%TypeCode
void fill_vardata_with_pyobj(vardata_t* d, PyObject* obj)
{
	if (PyFloat_Check(obj)) {
		*d = PyFloat_AsDouble(obj);
	}
	else if (PyLong_Check(obj)) {
        long long v = PyLong_AsLongLong(obj);
        if (v >= 0)
            *d = (unsigned int) v;
        else
            *d = (int) v;
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
	unsigned int as_uint() const;
	int as_int() const;
	
	//vardata_t& operator=(void* p);
	//vardata_t& operator=(const char* s);
	//vardata_t& operator=(double d);
	//vardata_t& operator=(unsigned int u);
	//vardata_t& operator=(int i);
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
			sipRes = PyLong_FromUnsignedLong(sipCpp->as_uint());
			break;
		case vardata_t::Integer:
			sipRes = PyLong_FromLong(sipCpp->as_int());
			break;
		case vardata_t::Bytes:
			sipRes = PyBytes_FromStringAndSize((const char*) sipCpp->as_bytes(), sipCpp->size());
			break;
	}
	%End

};