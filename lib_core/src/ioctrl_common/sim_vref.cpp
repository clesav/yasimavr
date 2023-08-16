/*
 * sim_vref.cpp
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

//=======================================================================================

#include "sim_vref.h"
#include "../core/sim_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

VREF::VREF(unsigned int ref_count)
:Peripheral(AVR_IOCTL_VREF)
,m_vcc(0.0)
,m_aref(0.0)
,m_references(ref_count)
{
    //Initialise the reference vector with a default value
    const ref_t ref_default = { 0.0, false };
    m_references.assign(ref_count, ref_default);

    //Ensures there's a valid value at the start for each reference
    //in the signal internal data map
    m_signal.set_data(Signal_VCCChange, 0.0);
    m_signal.set_data(Signal_ARefChange, 0.0);

    for (unsigned int i = 0; i < ref_count; ++i)
        m_signal.set_data(Signal_IntRefChange, 0.0, i);
}

bool VREF::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }
    else if (req == AVR_CTLREQ_VREF_GET) {
        if (!m_vcc)
            device()->crash(CRASH_INVALID_CONFIG, "VREF not set for analog operations.");
        else if (data->index == Source_VCC)
            data->data = m_vcc;
        else if (data->index == Source_AVCC)
            data->data = 1.0;
        else if (data->index == Source_AREF)
            data->data = m_aref;
        else if (data->index == Source_Internal) {
            unsigned int ref_index = data->data.as_uint();
            if (ref_index < m_references.size())
                data->data = reference(ref_index);
            else
                data->data = vardata_t();
        }
        return true;
    }

    else if (req == AVR_CTLREQ_VREF_SET) {

        if (data->index == Source_VCC) {
            //Get the new VCC value and ensure it's not negative
            m_vcc = data->data.as_double();
            if (m_vcc < 0.0) m_vcc = 0.0;

            m_signal.raise(Signal_VCCChange, m_vcc);

            //A VCC modif impacts all other references so we must
            //notify them all
            for (unsigned int i = 0; i < m_references.size(); ++i)
                m_signal.raise(Signal_IntRefChange, reference(i), i);

        }
        else if (data->index == Source_AREF) {
            //Get the new AREF value and bound it to the range [0.0; 1.0]
            m_aref = data->data.as_double();
            if (m_aref > 1.0) m_aref = 1.0;
            if (m_aref < 0.0) m_aref = 0.0;

            m_signal.raise(Signal_ARefChange, m_aref);
        }
        else
            return false;

        return true;
    }

    return false;
}

void VREF::set_reference(unsigned int index, Source source, double voltage)
{
    ref_t r;
    if (source == Source_VCC || source == Source_AVCC)
        r = { 1.0, true };
    else if (source == Source_AREF)
        r = { voltage, true };
    else
        r = { voltage, false };

    m_references[index] = r;

    if (m_vcc)
        m_signal.raise(Signal_IntRefChange, reference(index), index);
}

//Returns a reference as an absolute voltage value
double VREF::reference(unsigned int index) const
{
    if (index < m_references.size() && m_vcc) {
        ref_t r = m_references[index];
        double v;

        if (r.relative)
            v = r.value;
        else
            v = r.value / m_vcc;

        if (v > 1.0)
            v = 1.0;
        else if (v < 0.0)
            v = 0.0;

        return v;

    } else {
        return 0.0;
    }
}
