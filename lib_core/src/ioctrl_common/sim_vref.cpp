/*
 * sim_vref.cpp
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

//=======================================================================================

#include "sim_vref.h"
#include "../core/sim_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

static inline double constraint(double v)
{
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

VREF::VREF(unsigned int ref_count)
:Peripheral(AVR_IOCTL_VREF)
,m_vcc(0.0)
,m_aref(0.0)
,m_references(ref_count)
{
    ref_t r = ref_t{ Source_Bandgap, 0.0 };
    std::fill(m_references.begin(), m_references.end(), r);

    //Ensures there's a valid value at the start for each reference
    //in the signal internal data map
    m_signal.set_data(Signal_VCCChange, 0.0);
    m_signal.set_data(Signal_ARefChange, 0.0);

    for (unsigned int i = 0; i < ref_count; ++i)
        m_signal.set_data(Signal_IntRefChange, 0.0, i);
}


bool VREF::ctlreq(ctlreq_id_t req, ctlreq_data_t* reqdata)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        reqdata->data = &m_signal;
        return true;
    }

    else if (req == AVR_CTLREQ_VREF_GET) {
        if (!m_vcc) {
            device()->crash(CRASH_INVALID_CONFIG, "VREF not set for analog operations.");
            return false;
        }

        getset_t* info = reqdata->data.as_ptr<getset_t>();

        switch (info->source) {
            case Source_VCC:
                info->voltage = m_vcc; break;
            case Source_AVCC:
                info->voltage = 1.0; break;
            case Source_AREF:
                info->voltage = m_aref; break;
            case Source_Bandgap:
                info->voltage = constraint(m_references.at(info->channel).bandgap_voltage / m_vcc); break;
            case Source_Mux:
                info->voltage = reference(info->channel); break;
        }

        return true;
    }

    else if (req == AVR_CTLREQ_VREF_SET_REF) {
        getset_t* info = reqdata->data.as_ptr<getset_t>();

        if (!m_vcc && info->source != Source_VCC) {
            device()->crash(CRASH_INVALID_CONFIG, "VREF not set for analog operations.");
            return false;
        }

        switch(info->source) {
            case Source_VCC: {
                //Get the new VCC value and ensure it's not negative
                m_vcc = info->voltage;
                if (m_vcc < 0.0) m_vcc = 0.0;

                m_signal.raise(Signal_VCCChange, m_vcc);

                //A VCC modif impacts all other references so we must
                //notify them all
                for (unsigned int i = 0; i < m_references.size(); ++i)
                    m_signal.raise(Signal_IntRefChange, reference(i), i);
            } break;

            case Source_AREF: {
                //Get the new AREF value and bound it to the range [0.0; 1.0]
                m_aref = constraint(info->voltage);
                m_signal.raise(Signal_ARefChange, m_aref);
            } break;

            case Source_Bandgap: {
                m_references.at(info->channel).bandgap_voltage = info->voltage;
                if (m_references.at(info->channel).mux_source == Source_Bandgap)
                    m_signal.raise(Signal_IntRefChange, reference(info->channel), info->channel);
            } break;

            default:
                return false;
        }

        return true;
    }

    else if (req == AVR_CTLREQ_VREF_SET_MUX) {
        getset_t* info = reqdata->data.as_ptr<getset_t>();

        if (!m_vcc) {
            device()->crash(CRASH_INVALID_CONFIG, "VREF not set for analog operations.");
            return false;
        }

        m_references.at(info->channel).mux_source = info->source;
        if (info->source == Source_Bandgap && info->voltage >= 0.0)
            m_references.at(info->channel).bandgap_voltage = info->voltage;

        m_signal.raise(Signal_IntRefChange, reference(info->channel), info->channel);

        return true;
    }

    return false;
}

/**
   Set a voltage reference value as source for the mux of a channel
   \param index channel index of the reference
   \param source source of the reference
   \param voltage Value of the bandgap reference

   If source is any other than Bandgap, the voltage value is ignored.\n
   If source is Bandgap, voltage must be an absolute value in Volts.
 */
void VREF::set_reference(unsigned int index, Source source, double voltage)
{
    m_references[index].mux_source = source;
    if (source == Source_Bandgap && voltage >= 0.0)
        m_references[index].bandgap_voltage = voltage;

    if (m_vcc)
        m_signal.raise(Signal_IntRefChange, reference(index), index);
}

/**
   Set a voltage reference value as source for the mux of a channel
   \param index channel index of the reference
   \param source source of the reference

   If source is Bandgap, the voltage value is unchanged.
 */
void VREF::set_reference(unsigned int index, Source source)
{
    set_reference(index, source, m_references[index].bandgap_voltage);
}

/**
   Returns a voltage reference value.

   The value returned is always relative to VCC, even if set with an absolute value.\n
   The value is also constrained to the range [0; VCC].\n
   If index is out of range or VCC is not set, 0.0 is returned.
 */
double VREF::reference(unsigned int index) const
{
    if (index >= m_references.size() || !m_vcc)
        return 0.0;

    auto& r = m_references[index];
    switch (r.mux_source) {
        case Source_VCC:
        case Source_AVCC:
            return 1.0;

        case Source_AREF:
            return m_aref;

        case Source_Bandgap:
            return constraint(r.bandgap_voltage / m_vcc);

        default:
            return 0.0;
    }
}
