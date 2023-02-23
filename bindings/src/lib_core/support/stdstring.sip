/*
 * stdstring.sip
 *
 *  Copyright 2023 Clement Savergne <csavergne@yahoo.com>

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

%MappedType std::string
    /TypeHintIn="str",
     TypeHintOut="str"/
{
%TypeHeaderCode
#include <string>
%End

%ConvertFromTypeCode
    return PyUnicode_FromString(sipCpp->c_str());
%End

%ConvertToTypeCode
    if (!sipIsErr) {
        return PyUnicode_Check(sipPy);
    } else {
        (*sipCppPtr) = new std::string(PyUnicode_AsUTF8(sipPy));
        return 0;
    }
%End
};
