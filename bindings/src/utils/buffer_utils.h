/*
 * buffer_utils.h
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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

uint32_t import_from_pybuffer(const sipAPIDef* sipAPI,
							  uint8_t **data,
							  PyObject* exporter);

PyObject* export_to_pybuffer(const sipAPIDef* sipAPI,
							 uint8_t *data,
							 uint32_t len);

int import_from_fixedlen_sequence(const sipAPIDef* sipAPI,
								  void *data,
								  const char *format,
								  Py_ssize_t len,
								  PyObject* obj);

#ifdef __cplusplus
}
#endif

#endif //__PYBUFFER_SUPPORT_H__
