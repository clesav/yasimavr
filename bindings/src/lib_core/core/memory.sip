/*
 * memory.sip
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

class NonVolatileMemory /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_memory.h"
%End
%TypeCode
#include "utils/buffer_utils.h"
%End

public:

    NonVolatileMemory(size_t, const std::string = "");
    NonVolatileMemory(const NonVolatileMemory&);

    size_t size() const;
    const std::string& name() const;

    bool programmed(size_t) const;
    SIP_PYOBJECT programmed(size_t, size_t) const /TypeHint="bytes"/;
    %MethodCode
        if (a1) {
            unsigned char* buf = (unsigned char*) sipMalloc(a1);
            size_t len_res = sipCpp->programmed(buf, a0, a1);
            sipRes = export_to_pybuffer(sipAPI_core, buf, len_res);
            sipFree(buf);
        } else {
            sipRes = PyBytes_FromString("");
        }
    %End

    unsigned char operator[](size_t) const /PyInt/;

    SIP_PYOBJECT block() const /TypeHint="bytes"/;
    %MethodCode
        mem_block_t b = sipCpp->block();
        sipRes = export_to_pybuffer(sipAPI_core, b.buf, b.size);
    %End

    SIP_PYOBJECT block(size_t, size_t) const /TypeHint="bytes"/;
    %MethodCode
        mem_block_t b = sipCpp->block(a0, a1);
        sipRes = export_to_pybuffer(sipAPI_core, b.buf, b.size);
    %End

    bool program(SIP_PYBUFFER, size_t = 0);
    %MethodCode
        unsigned char* buf;
        size_t len = import_from_pybuffer(sipAPI_core, &buf, a0);
        if (len) {
            sipCpp->program({len, buf}, a1);
            sipFree(buf);
        }
    %End

    void erase();
    void erase(size_t, size_t);
    void erase(SIP_PYBUFFER, size_t);
    %MethodCode
        unsigned char* buf;
        size_t len = import_from_pybuffer(sipAPI_core, &buf, a0);
        if (len) {
            sipCpp->erase(buf, a1, len);
            sipFree(buf);
        }
    %End

    int dbg_read(size_t) const;
    SIP_PYOBJECT dbg_read(size_t, size_t) const /TypeHint="bytes"/;
    %MethodCode
        if (a1) {
            unsigned char* buf = (unsigned char*) sipMalloc(a1);
            size_t len_res = sipCpp->dbg_read(buf, a0, a1);
            sipRes = export_to_pybuffer(sipAPI_core, buf, len_res);
            sipFree(buf);
        } else {
            sipRes = PyBytes_FromString("");
        }
    %End

    void dbg_write(unsigned char /PyInt/, size_t);
    void dbg_write(SIP_PYBUFFER, size_t);
    %MethodCode
        unsigned char* buf;
        size_t len = import_from_pybuffer(sipAPI_core, &buf, a0);
        if (len) {
            sipCpp->dbg_write(buf, a1, len);
            sipFree(buf);
        }
    %End

    void spm_write(unsigned char /PyInt/, size_t);
    void spm_write(SIP_PYBUFFER, SIP_PYBUFFER, size_t);
    %MethodCode
        unsigned char* buf;
        size_t len = import_from_pybuffer(sipAPI_core, &buf, a0);

        unsigned char* bufset = nullptr;
        if (a1) {
            size_t lenset = import_from_pybuffer(sipAPI_core, &bufset, a1);
            if (lenset < len) len = lenset;
        }

        sipCpp->spm_write(buf, bufset, a2, len);

        if (buf)
            sipFree(buf);
        if (bufset)
            sipFree(bufset);
    %End

};
