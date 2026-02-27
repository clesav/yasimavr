/*
 * buffer_utils.h
 *
 *  Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __BUFFER_UTILS_H__
#define __BUFFER_UTILS_H__

#include "Python.h"
#include "sip.h"

/*
 * Helper function for importing data from a fixed-length
 * Python object that supports the Sequence protocol
 */
inline int convert_to_fixed_array(void *data, const char *format, size_t len, PyObject* obj)
{
    if (!PySequence_Check(obj) || (PySequence_Size(obj) != len)) {
        PyErr_Format(PyExc_TypeError, "obj must be a sequence of length %d", len);
        return -1;
    }

    PyObject* arr = sipConvertToArray(data, format, len, 0);
    if (!arr)
        return -1;

    for (unsigned int i = 0; i < len; ++i) {
        PyObject* item = PySequence_GetItem(obj, i);
        if (item) {
            PyObject_SetItem(arr, PyLong_FromLong(i), item);
            Py_DECREF(item);
        }
        if (PyErr_Occurred()) break;
    }

    Py_DECREF(arr);

    return PyErr_Occurred() ? -1 : 0;
}


template<typename ItemType>
int convert_to_fixed_array(sipTypeDef* type_def, ItemType* data, size_t len, PyObject* obj)
{
    if (!PySequence_Check(obj) || (PySequence_Size(obj) != len)) {
        PyErr_Format(PyExc_TypeError, "Value must be a sequence of length %d", len);
        return -1;
    }

    int iserr = 0;
    for (unsigned int i = 0; i < len; ++i) {
        PyObject* item = PySequence_GetItem(obj, i);
        if (!item) {
            iserr = -1;
            break;
        };

        int conv_state;
        ItemType* itemCpp = reinterpret_cast<ItemType*>(sipForceConvertToType(item, type_def, NULL, 0, &conv_state, &iserr));
        Py_DECREF(item);
        if (iserr) break;

        data[i] = *itemCpp;
        sipReleaseType(itemCpp, type_def, conv_state);

    }

    return iserr;
}


inline PyObject* convert_from_fixed_array(void* data, const char *format, size_t len)
{
    return sipConvertToArray(data, format, len, 0);
}


template<typename ItemType>
PyObject* convert_from_fixed_array(const sipTypeDef* type_def, ItemType* data, size_t len)
{
    PyObject* tuple = PyTuple_New(len);
    if (PyErr_Occurred())
        return NULL;

    for (int i = 0; i < len; i++) {
        PyObject* item = sipConvertFromType(data + i, type_def, Py_None);
        PyTuple_SET_ITEM(tuple, i, item);
    }

    return tuple;
}


#endif //__PYBUFFER_SUPPORT_H__
