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
inline int import_from_fixedlen_sequence(const sipAPIDef* sipAPI,
                                  void *data,
                                  const char *format,
                                  Py_ssize_t len,
                                  PyObject* obj)
{
    if (!PySequence_Check(obj) || (PySequence_Size(obj) != len)) {
        PyErr_Format(PyExc_TypeError, "obj must be a sequence of length %d", len);
        return -1;
    }

    PyObject* arr = sipAPI->api_convert_to_array(data, format, len, 0);
    if (!arr)
        return -1;

    for (int i = 0; i < len; ++i) {
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

#endif //__PYBUFFER_SUPPORT_H__
