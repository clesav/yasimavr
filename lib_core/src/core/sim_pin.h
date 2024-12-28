/*
 * sim_pin.h
 *
 *  Copyright 2021-2024 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_PIN_H__
#define __YASIMAVR_PIN_H__

#include "sim_wire.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

typedef sim_id_t pin_id_t;


class PinManager;


/**
   \brief MCU pin model.

   Pin represents a external pad of the MCU used for GPIO.
   The pin has two electrical states given by the external circuit and the internal circuit (the GPIO port),
   which are resolved into a single electrical state. In case of conflict, the SHORTED state is
   set.
   The internal circuit state is set by GPIO controls.
 */
class AVR_CORE_PUBLIC_API Pin : public Wire {

public:

    /**
       Structure for defining the controls of a GPIO
     */
    struct controls_t {

        /// Direction control: 0=IN, 1=OUT
        unsigned char dir = 0;
        /// Driving state: 0=LOW, 1=HIGH
        unsigned char drive = 0;
        /// Enable inversion of the driving state
        bool inverted = false;
        /// Enable the pull up
        bool pull_up = false;

    };

    explicit Pin(pin_id_t id);

    pin_id_t id() const;

    void set_external_state(const state_t& state);
    void set_external_state(StateEnum state, double level = 0.0);
    void set_external_state(char state, double level = 0.0);

    void set_gpio_controls(const controls_t& controls);
    controls_t gpio_controls() const;

    Pin(const Pin&) = delete;
    Pin& operator=(const Pin&) = delete;

private:

    friend class PinManager;
    friend class PinDriver;

    pin_id_t m_id;
    controls_t m_gpio_controls;
    state_t m_gpio_state;
    PinManager* m_manager;

    virtual void notify_digital_state(bool state) override;
    virtual state_t state_for_resolution() const override;

    void update_pin_state();

};


inline pin_id_t Pin::id() const
{
    return m_id;
}

/**
   Set the state applied to the pin by the external circuit
 */
inline void Pin::set_external_state(const state_t& state)
{
    set_state(state);
}

/**
   Set the state applied to the pin by the external circuit
 */
inline void Pin::set_external_state(StateEnum state, double level)
{
    set_state(state, level);
}

/**
   Set the state applied to the pin by the external circuit
 */
inline void Pin::set_external_state(char state, double level)
{
    set_state(state, level);
}

/**
   Getter for the GPIO controls currently applied
 */
inline Pin::controls_t Pin::gpio_controls() const
{
    return m_gpio_controls;
}


//=======================================================================================

/**
   \brief MCU pin driver.

   PinDriver is an interface that allows to override the controls of a MCU pin.
   It is usually used as a sub-object of a peripheral that, under some conditions,
   takes control of a GPIO.
   The PinDriver does not know which pins it controls. The driver only references pins by a
   arbitrary integer index (0 to N) that has meaning only for the driver.

   To operate, a driver must be registered with the PinManager object during the device initialisation
   phase.

   \sa Pin, PinManager
 */
class AVR_CORE_PUBLIC_API PinDriver {

public:

    typedef unsigned int pin_index_t;

    PinDriver(ctl_id_t id, pin_index_t pin_count);
    virtual ~PinDriver();

    void set_enabled(bool enabled);
    void set_enabled(pin_index_t index, bool enabled);
    bool enabled(pin_index_t index) const;

    void update_pin_state(pin_index_t pin_index);
    void update_pin_states();

    Wire::state_t pin_state(pin_index_t pin_index) const;
    Pin::controls_t gpio_controls(pin_index_t pin_index) const;

    PinDriver(const PinDriver&) = delete;
    PinDriver& operator=(const PinDriver&) = delete;

    /**
       Stub called when a state resolution is taking place on a pin that the driver is controlling.
       The reimplementation should make a copy of the controls structure in argument, change its members
       according to the override state and return the result.
       \param pin_index index of the pin
       \param controls pin controls as configured by the GPIO port controller
       \return the controls to apply to the pin
     */
    virtual Pin::controls_t override_gpio(pin_index_t pin_index, const Pin::controls_t& controls) = 0;

    virtual void digital_state_changed(pin_index_t pin_index, bool state);

private:

    friend class PinManager;

    ctl_id_t m_id;
    PinManager* m_manager;
    pin_index_t m_pin_count;
    Pin** m_pins;

};


//=======================================================================================

/**
   \brief MCU pin manager.

   Class managing a set of pins and the mux configurations between the pin drivers and the
   pins.

   \sa Pin, PinDriver
 */
class AVR_CORE_PUBLIC_API PinManager {

public:

    typedef int mux_index_t;

    explicit PinManager(const std::vector<pin_id_t>& pin_ids);
    ~PinManager();

    bool register_driver(PinDriver& drv);
    bool add_mux_config(ctl_id_t drv, const std::vector<pin_id_t>& pins, mux_index_t mux_index = 0);

    void set_current_mux(ctl_id_t drv, mux_index_t index);
    mux_index_t current_mux_index(ctl_id_t drv) const;
    std::vector<pin_id_t> current_mux_pins(ctl_id_t drv) const;

    Pin* pin(pin_id_t pin_id) const;

    PinManager(const PinManager&) = delete;
    PinManager& operator=(const PinManager&) = delete;

private:

    friend class Pin;
    friend class PinDriver;

    struct pin_entry_t;
    struct drv_entry_t;

    std::unordered_map<pin_id_t, pin_entry_t*> m_pins;
    std::unordered_map<ctl_id_t, drv_entry_t*> m_drivers;

    void add_driver_to_pin(pin_entry_t& pin_entry, PinDriver& drv, PinDriver::pin_index_t index);
    void remove_driver_from_pin(pin_entry_t& pin_entry, PinDriver& drv, PinDriver::pin_index_t index);

    Wire::state_t override_gpio(pin_id_t pin_id, const Pin::controls_t& gpio_controls);
    void notify_digital_state(pin_id_t pin_id, bool state);
    void set_driver_enabled(PinDriver& drv, PinDriver::pin_index_t index, bool enabled);
    bool driver_enabled(const PinDriver& drv, PinDriver::pin_index_t index) const;
    void unregister_driver(PinDriver& drv);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_PIN_H__
