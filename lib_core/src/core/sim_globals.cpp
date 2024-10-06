/*
 * sim_globals.cpp
 *
 *  Copyright 2024 Clement Savergne <csavergne@yahoo.com>

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

#include "sim_globals.h"

/*
  Declare the version constants in both number and string format.
  The actual value comes either from :
  - define arguments given to the compiler (method used by the 'setup.py' script), or
  - from the "sim_version.h" generated automatically (method used by the makefiles)
 */

#if !defined(YASIMAVR_VERSION) or !defined(YASIMAVR_VERSION_STR)
#include "sim_version.h"
#endif
//If both methods failed to define the constants, throw a compilation error
#if !defined(YASIMAVR_VERSION) or !defined(YASIMAVR_VERSION_STR)
#error "Version constants not defined"
#endif


#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)


YASIMAVR_USING_NAMESPACE

const unsigned long LIB_VERSION = YASIMAVR_VERSION;
const char* LIB_VERSION_STR = TO_STRING(YASIMAVR_VERSION_STR);
