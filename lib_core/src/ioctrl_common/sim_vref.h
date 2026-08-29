/*
 * sim_vref.h
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
   `data` should point to a VREF::getset_t object, with `source` set to the required source (one of VREF::Source enum values)\n
   For Bandgap or Mux references, `channel` shall be set to the required channel.\n
   The voltage value is stored in `voltage`, as an absolute value in volts for VCC, and a relative value to VCC
   for all others.
 */
#define AVR_CTLREQ_VREF_GET             (AVR_CTLREQ_BASE + 1)

/**
   Request to set VCC, AREF or a bandgap reference values.\n
   `data` should point to a VREF::getset_t object, with `source` set to the required source (one of VREF::Source enum values,
   but only VCC, VREF or Bandgap are accepted)\n
   For Bandgap, `channel` shall be set to the required channel.\n
   `voltage` shall be set to the absolute value in volts.
 */
#define AVR_CTLREQ_VREF_SET_REF         (AVR_CTLREQ_BASE + 2)

/**
   Request to change the source of the mux for a given channel
   `source` set to the required source (one of VREF::Source enum values, but only VCC, VREF or Bandgap are accepted)\n
   For Bandgap, `channel` shall be set to the required channel.\n
   `voltage` shall be set to the absolute value in volts.
 */
#define AVR_CTLREQ_VREF_SET_MUX         (AVR_CTLREQ_BASE + 3)

/**
   Request to set the value reported by the simulated temperature sensor
    - data set to the temperature value in °C (as a double)
 */
#define AVR_CTLREQ_VREF_SET_TEMP        (AVR_CTLREQ_BASE + 4)

/// @}
/// @}


//=======================================================================================
/**
   \brief Configuration structure for a generic sleep mode controller.
   \sa VREF
 */
struct VREFConfig {

    /// Calibration value for the internal temperature sensor - offset in V at +25°C
    double temp_cal_25C;
    /// Calibration value for the internal temperature sensor - linear coefficient in V/°C
    double temp_cal_coef;

};


/**
   \ingroup api_vref
   \brief Generic model for managing VREF for analog peripherals (ADC, analog comparator)
   \note Setting VCC in the firmware is required for using any analog feature of a MCU.
   Failing to do so will trigger a device crash.
 */
class AVR_CORE_PUBLIC_API VREF : public Peripheral {

public:

    /// Enumeration value for the sources of voltage references
    enum Source {
        Source_VCC,             ///< VCC voltage value
        Source_AVCC,            ///< AVCC voltage value (always equal to VCC for now)
        Source_AREF,            ///< AREF voltage value
        Source_Bandgap,         ///< Internal reference voltage value
        Source_Temperature,     ///< Temperature sensor
        Source_Mux,             ///< Reference mux selection
    };

    enum SignalId {
        /**
           Raised when the AREF reference value is changed. data carries the new value (absolute)
         */
        Signal_ARefChange,
        /**
           Raised when an internal reference value is changed.
           data carries the new value (relative to VCC) and index the reference channel.
         */
        Signal_IntRefChange,
        /**
           Raised when VCC value is changed.
           data carries the new value (absolute)
         */
        Signal_VCCChange,
        /**
           Raised when the temperature sensor output has changed.
           data carries the new value (relative to VCC)
         */
        Signal_TempChange,
    };

    struct getset_t {
        Source source;
        unsigned int channel = 0;
        double voltage = -1.0;
    };

    VREF(const VREFConfig& config, unsigned int ref_count);

    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* reqdata) override;

protected:

    void set_reference(unsigned int index, Source source);
    void set_reference(unsigned int index, Source source, double voltage);
    double reference(unsigned int index) const;

    const VREFConfig& m_config;

private:

    double m_vcc;
    double m_aref;
    DataSignal m_signal;
    double m_temperature;

    struct ref_t {
        Source mux_source;
        double bandgap_voltage;
    };

    std::vector<ref_t> m_references;

    double temperature_reference() const;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_VREF_H__
