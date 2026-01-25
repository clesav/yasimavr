/*
 * stdbytes.sip
 *
 *  Copyright 2026 Clement Savergne <csavergne@yahoo.com>

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

%MappedType bytes_t
    /TypeHintIn="buffer", TypeHintOut="bytes"/
{
%TypeHeaderCode
#include "core/sim_types.h"
%End

%ConvertFromTypeCode
    bytes_view_t bv = *sipCpp;
    return bytes_to_pybytes(bv);
%End

%ConvertToTypeCode
    if (!sipIsErr)
        return PyObject_CheckBuffer(sipPy);

    bytes_t b = pybuffer_to_bytes(sipPy);
    *sipCppPtr = new bytes_t(std::move(b));
    return sipGetState(sipTransferObj);
%End
};


//=======================================================================================

%MappedType bytes_view_t
    /TypeHintIn="buffer", TypeHintOut="memoryview"/
{
%TypeHeaderCode
#include "core/sim_types.h"
%End

%ConvertFromTypeCode
    return bytesview_to_pyview(*sipCpp);
%End

%ConvertToTypeCode
    if (!sipIsErr)
        return PyObject_CheckBuffer(sipPy);

    //Create a private memoryview object that will live as long as the bytes_view_t object lives
    //The SIP UserState facility is used to stored the pointer to this memoryview object and is dereferenced
    //in the release code.
    PyObject* py_view = PyMemoryView_GetContiguous(sipPy, PyBUF_READ, 'C');
    *sipUserStatePtr = sipMalloc(sizeof(PyObject*));
    **(PyObject***)sipUserStatePtr = py_view;

    Py_buffer* py_buf = PyMemoryView_GET_BUFFER(py_view);
    *sipCppPtr = new bytes_view_t({ (uint8_t*) py_buf->buf, (size_t) py_buf->len });

    return sipGetState(sipTransferObj) | SIP_USER;
%End

%ReleaseCode
    delete sipCpp;
    //dereference the private memoryview object whose pointer is stored in the heap pointed by UserState (a pointer)
    if (sipState & SIP_USER) {
        Py_DECREF(*(PyObject**)sipUserState);
        sipFree(sipUserState);
    }
%End

};


//=======================================================================================

%ModuleHeaderCode

#include "core/sim_types.h"

/*
 * Helper that copies a block of bytes from a from a Python object
 * supporting the buffer protocol (see Python docs for what that means)
 * into an C++ bytes_t object (a.k.a vector of unsigned char).
 */
bytes_t pybuffer_to_bytes(PyObject* exporter);

/*
 * Helper that copies a block of bytes from a from a byte_view_t object
 * to a Python bytes object.
 */
PyObject* bytes_to_pybytes(const bytes_view_t& bv);

/*
 * Helper that creates a Python 'memoryview' object pointing to the same data as
 * a C++ bytes_view_t object.
 */
PyObject* bytesview_to_pyview(const bytes_view_t& b);

%End

%ModuleCode

bytes_t pybuffer_to_bytes(PyObject* exporter)
{
    if (!exporter)
        return bytes_t();

    Py_buffer buffer;
    int ok = PyObject_GetBuffer(exporter, &buffer, PyBUF_SIMPLE);
    if (ok < 0)
        return bytes_t();

    bytes_t b = bytes_t(buffer.len, 0);
    if (buffer.len)
        PyBuffer_ToContiguous(b.data(), &buffer, buffer.len, 'C');

    PyBuffer_Release(&buffer);

    return b;
}


PyObject* bytes_to_pybytes(const bytes_view_t& bv)
{
    if (bv.size())
        return PyBytes_FromStringAndSize((const char*) bv.data(), bv.size());
    else
        return PyBytes_FromString("");
}


PyObject* bytesview_to_pyview(const bytes_view_t& b)
{
    return PyMemoryView_FromMemory((char*) b.data(), b.size(), PyBUF_READ);
}

%End
