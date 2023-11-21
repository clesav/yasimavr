/*
 * buffer_utils.h
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

#ifndef __BUFFER_UTILS_H__
#define __BUFFER_UTILS_H__

#include "Python.h"
#include "sip.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Helper that imports an block of bytes from a Python object
 * supporting the buffer protocol (see Python docs for what that means)
 * into an C array.
 * buf is a pointer to a array pointer, that is allocated/freed on demand
 */
Py_ssize_t import_from_pybuffer(const sipAPIDef* sipAPI,
                                uint8_t **data,
                                PyObject* exporter);

/*
 * Helper that exports a block of bytes in memory to a Python buffer exporter
 * If len ==0 or data is NULL, an empty Python bytes object is returned
 * otherwise a SIP array object is created
 */
PyObject* export_to_pybuffer(const sipAPIDef* sipAPI,
                             const uint8_t *data,
                             Py_ssize_t len);

/*
 * Helper function for importing data from a fixed-length
 * Python object that supports the Sequence protocol
 */
int import_from_fixedlen_sequence(const sipAPIDef* sipAPI,
                                  void *data,
                                  const char *format,
                                  Py_ssize_t len,
                                  PyObject* obj);

#ifdef __cplusplus
}
#endif

#endif //__PYBUFFER_SUPPORT_H__
