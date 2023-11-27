/*
 * buffer_utils.c
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

#include "buffer_utils.h"


Py_ssize_t import_from_pybuffer(const sipAPIDef* sipAPI, uint8_t **buf, PyObject* exporter)
{
    Py_buffer buffer;
    Py_ssize_t len;

    PyObject_GetBuffer(exporter, &buffer, PyBUF_SIMPLE);
    len = buffer.len;

    if (len) {
        *buf = (uint8_t*) sipAPI->api_malloc(len);
        PyBuffer_ToContiguous(*buf, &buffer, len, 'C');
    } else {
        *buf = NULL;
    }

    PyBuffer_Release(&buffer);
    return len;
}


PyObject* export_to_pybuffer(const sipAPIDef* sipAPI, const uint8_t *data, Py_ssize_t len)
{
    if (len > 0 && data) {
        void* buf = sipAPI->api_malloc(len);
        memcpy(buf, data, len);
        return sipAPI->api_convert_to_array(buf, "B", len, SIP_OWNS_MEMORY);
    } else {
        return PyBytes_FromString("");
    }
}


int import_from_fixedlen_sequence(const sipAPIDef* sipAPI,
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
