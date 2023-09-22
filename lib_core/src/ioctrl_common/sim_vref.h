/*
 * sim_vref.h
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

#ifndef __YASIMAVR_VREF_H__
#define __YASIMAVR_VREF_H__

#include "../core/sim_peripheral.h"
#include "../core/sim_types.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \file
   \defgroup api_vref Voltage Reference framework
   @{
 */

/**
   \name Controller requests definition for VREF
   @{
 */

/**
   Request to interrogate the VREF controller and obtain a reference value.\n
   The index shall be set to the required source (one of VREF::Source enum values)\n
   For Internal references, data shall be set to the required channel, as an unsigned integer.\n
   On return, data is set to the reference value as a double. Except for VCC, all values are relative to VCC.
 */
#define AVR_CTLREQ_VREF_GET             1

/**
   Request to set VCC or AREF reference values.\n
   The index shall be set to the required source (one of VREF::Source enum values but only VCC and VREF are accepted)\n
   data shall be set to the required value as a double.\n
   VCC shall be an absolute value in Volts, AREF shall be relative to VCC and contrainted to the range [0; 1].
 */
#define AVR_CTLREQ_VREF_SET             2

/// @}
/// @}


//=======================================================================================
/**
   \ingroup api_vref
   \brief Generic model for managing VREF for analog peripherals (ADC, analog comparator)
   \note Setting VCC in the firmware is required for using any analog feature of a MCU.
   Failing to do so will trigger a device crash.
 */
class AVR_CORE_PUBLIC_API VREF : public Peripheral {

public:

    /// Enumation value for the sources of voltage references
    enum Source {
        Source_VCC,             ///< VCC voltage value
        Source_AVCC,            ///< AVCC voltage value (always equal to VCC for now)
        Source_AREF,            ///< AREF voltage value
        Source_Internal,        ///< Internal reference voltage value
    };

    enum SignalId {
        /**
           Raised when the AREF reference value is changed. data carries the new value (absolute)
         */
        Signal_ARefChange,
        /**
           Raised when an internal reference value is changed.
           data carries the new value (relative to VCC) and index the reference index.
         */
        Signal_IntRefChange,
        /**
           Raised when VCC value is changed.
           data carries the new value (absolute)
         */
        Signal_VCCChange,
    };

    explicit VREF(unsigned int ref_count);

    bool active() const;

    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;

protected:

    void set_reference(unsigned int index, Source source, double voltage=1.0);
    double reference(unsigned int index) const;

private:

    double m_vcc;
    double m_aref;
    DataSignal m_signal;

    struct ref_t {
        double value;
        bool relative;
    };

    std::vector<ref_t> m_references;

};

inline bool VREF::active() const
{
    return m_vcc;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_VREF_H__
