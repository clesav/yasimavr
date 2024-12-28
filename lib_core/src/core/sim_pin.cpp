/*
 * sim_pin.cpp
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

#include "sim_pin.h"
#include <set>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

static Wire::state_t controls_to_state(const Pin::controls_t& c)
{
    Wire::state_t s;
    if (c.dir) {
        if (c.drive ^ c.inverted)
            s = Wire::State_High;
        else
            s = Wire::State_Low;
    } else {
        if (c.pull_up)
            s = Wire::State_PullUp;
        else
            s = Wire::State_Floating;
    }

    return s;
}


Pin::Pin(pin_id_t id)
:Wire()
,m_id(id)
,m_manager(nullptr)
{}


/**
   Set the controls for the internal circuit state
 */
void Pin::set_gpio_controls(const controls_t& controls)
{
    m_gpio_controls = controls;
    update_pin_state();
}


void Pin::notify_digital_state(bool state)
{
    if (m_manager)
        m_manager->notify_digital_state(m_id, state);
}


Wire::state_t Pin::state_for_resolution() const
{
    state_t ext_pin_state = Wire::state_for_resolution();
    return resolve_two_states(ext_pin_state, m_gpio_state);
}


void Pin::update_pin_state()
{
    if (m_manager)
        m_gpio_state = m_manager->override_gpio(m_id, m_gpio_controls);
    else
        m_gpio_state = controls_to_state(m_gpio_controls);

    auto_resolve_state();
}


//=======================================================================================

/**
   Construct a pin driver.
   \param id Driver ID, usually the same ID as a peripheral controller the driver
   \param pin_count number of pin driven by this driver
 */
PinDriver::PinDriver(ctl_id_t id, pin_index_t pin_count)
:m_id(id)
,m_manager(nullptr)
,m_pin_count(pin_count)
,m_pins(nullptr)
{
    if (pin_count) {
        m_pins = new Pin*[pin_count];
        std::fill(m_pins, m_pins + pin_count, nullptr);
    }
}


PinDriver::~PinDriver()
{
    if (m_manager)
        m_manager->unregister_driver(*this);

    if (m_pin_count)
        delete[] m_pins;
}

void PinDriver::set_enabled(bool enabled)
{
    for (pin_index_t i = 0; i < m_pin_count; ++i)
        set_enabled(i, enabled);
}


void PinDriver::set_enabled(pin_index_t pin_index, bool enabled)
{
    if (m_manager)
        m_manager->set_driver_enabled(*this, pin_index, enabled);
}


bool PinDriver::enabled(pin_index_t pin_index) const
{
    if (m_manager)
        return m_manager->driver_enabled(*this, pin_index);
    else
        return false;
}


void PinDriver::update_pin_state(pin_index_t pin_index)
{
    if (pin_index < m_pin_count && m_pins[pin_index])
        m_pins[pin_index]->update_pin_state();
}


void PinDriver::update_pin_states()
{
    for (pin_index_t i = 0; i < m_pin_count; ++i)
        if (m_pins[i])
            m_pins[i]->update_pin_state();
}


Wire::state_t PinDriver::pin_state(pin_index_t pin_index) const
{
    if (pin_index < m_pin_count && m_pins[pin_index])
        return m_pins[pin_index]->state();
    else
        return Wire::State_Error;
}


Pin::controls_t PinDriver::gpio_controls(pin_index_t pin_index) const
{
    if (pin_index < m_pin_count && m_pins[pin_index])
        return m_pins[pin_index]->gpio_controls();
    else
        return Pin::controls_t();
}


void PinDriver::digital_state_changed(pin_index_t pin_index, bool state)
{}


//=======================================================================================

struct PinManager::pin_entry_t {

    Pin pin;
    std::set<std::pair<PinDriver*, PinDriver::pin_index_t>> drivers;

    pin_entry_t(pin_id_t id)
    :pin(id)
    {}

};


struct PinManager::drv_entry_t {

    PinDriver& driver;
    std::unordered_map<mux_index_t, pin_id_t*> mux_configs;
    bool* enabled_pins;
    mux_index_t current_mux_index;

    explicit drv_entry_t(PinDriver& drv)
    :driver(drv)
    ,current_mux_index(-1)
    {
        enabled_pins = new bool[drv.m_pin_count];
        std::fill(enabled_pins, enabled_pins + drv.m_pin_count, false);
    }

    ~drv_entry_t()
    {
        delete[] enabled_pins;
        for (auto [mux_index, pins] : mux_configs)
            delete[] pins;
    }

    void add_mux(mux_index_t mux_index, const std::vector<pin_id_t>& pins)
    {
        PinDriver::pin_index_t n = pin_count();
        pin_id_t* mux_config = new pin_id_t[n];
        memcpy(mux_config, pins.data(), n * sizeof(pin_id_t));
        mux_configs[mux_index] = mux_config;
    }

    bool has_mux(mux_index_t mux_index) const
    {
        return mux_configs.find(mux_index) != mux_configs.end();
    }

    inline pin_id_t pin_id(PinDriver::pin_index_t pin_index, mux_index_t mux_index) const
    {
        return mux_configs.at(mux_index)[pin_index];
    }

    inline PinDriver::pin_index_t pin_count() const
    {
        return driver.m_pin_count;
    }

};


PinManager::PinManager(const std::vector<pin_id_t>& pin_ids)
{
    for (pin_id_t i : pin_ids)
        m_pins[i] = new pin_entry_t(i);
}


PinManager::~PinManager()
{
    for (auto& [i, pin_entry] : m_pins) {
        pin_entry->pin.m_manager = nullptr;
        delete pin_entry;
    }

    for (auto& [i, drv_entry] : m_drivers) {
        drv_entry->driver.m_manager = nullptr;
        delete drv_entry;
    }
}


bool PinManager::register_driver(PinDriver& drv)
{
    if (m_drivers.count(drv.m_id) || !drv.m_pin_count || drv.m_manager) return false;

    drv_entry_t* drv_entry = new drv_entry_t(drv);
    m_drivers[drv.m_id] = drv_entry;

    drv.m_manager = this;

    return true;
}


bool PinManager::attach_driver(ctl_id_t drv_id, const std::vector<pin_id_t>& pin_ids, mux_index_t mux_index)
{
    if (!m_drivers.count(drv_id)) return false;
    drv_entry_t* drv_entry = m_drivers.at(drv_id);

    if (pin_ids.size() != drv_entry->pin_count()) return false;

    for (size_t i =  0; i < pin_ids.size(); ++i)
        if (!m_pins.count(pin_ids[i])) return false;

    if (mux_index < 0 || drv_entry->has_mux(mux_index)) return false;

    drv_entry->add_mux(mux_index, pin_ids);

    //If this is the first mux config for this driver, make it current
    if (drv_entry->mux_configs.size() == 1)
        set_current_mux(drv_id, mux_index);

    return true;
}


PinManager::mux_index_t PinManager::current_mux_index(ctl_id_t drv_id) const
{
    auto it = m_drivers.find(drv_id);
    if (it == m_drivers.end()) return -1;
    return it->second->current_mux_index;
}


std::vector<pin_id_t> PinManager::current_mux_pins(ctl_id_t drv_id) const
{
    auto it = m_drivers.find(drv_id);
    if (it == m_drivers.end()) return std::vector<pin_id_t>();
    drv_entry_t* drv_entry = it->second;

    if (drv_entry->current_mux_index >= 0) {
        pin_id_t* mux_config = drv_entry->mux_configs.at(drv_entry->current_mux_index);
        return std::vector<pin_id_t>(mux_config, mux_config + drv_entry->pin_count());
    } else {
        return std::vector<pin_id_t>(drv_entry->pin_count(), 0);
    }
}


void PinManager::set_current_mux(ctl_id_t drv_id, mux_index_t mux_index)
{
    drv_entry_t* drv_entry = m_drivers.at(drv_id);
    if (!drv_entry || (mux_index >= 0 && !drv_entry->has_mux(mux_index))) return;

    //Disconnect the driver from the pins in the previous mux configuration if it was valid
    if (drv_entry->current_mux_index >= 0) {
        for (PinDriver::pin_index_t pin_index = 0; pin_index < drv_entry->pin_count(); ++pin_index) {
            pin_id_t old_pin_id = drv_entry->pin_id(pin_index, drv_entry->current_mux_index);
            if (old_pin_id) {
                drv_entry->driver.m_pins[pin_index] = nullptr;
                if (drv_entry->enabled_pins[pin_index])
                    remove_driver_from_pin(*m_pins.at(old_pin_id), drv_entry->driver, pin_index);
            }
        }
    }

    //Connect the driver to the new pins in the new mux configuration if it is valid
    if (mux_index >= 0) {
        for (PinDriver::pin_index_t pin_index = 0; pin_index < drv_entry->pin_count(); ++pin_index) {
            pin_id_t new_pin_id = drv_entry->pin_id(pin_index, mux_index);
            if (new_pin_id) {
                pin_entry_t* pin_entry = m_pins.at(new_pin_id);
                drv_entry->driver.m_pins[pin_index] = &pin_entry->pin;
                if (drv_entry->enabled_pins[pin_index])
                    add_driver_to_pin(*pin_entry, drv_entry->driver, pin_index);
            }
        }
    }

    drv_entry->current_mux_index = mux_index;
}


void PinManager::add_driver_to_pin(pin_entry_t& pin_entry, PinDriver& drv, PinDriver::pin_index_t index)
{

    //Add the driver pointer to the pin attributes
    pin_entry.drivers.insert({ &drv, index });
    //Add the manager to the pin attributes (if not already present)
    pin_entry.pin.m_manager = this;
    //Update the pin state
    pin_entry.pin.update_pin_state();
}


void PinManager::remove_driver_from_pin(pin_entry_t& pin_entry, PinDriver& drv, PinDriver::pin_index_t index)
{
    //Remove the driver pointer from the pin attributes
    pin_entry.drivers.erase({ &drv, index });
    //Remove the manager from the pin attributes if there's no driver left
    if (!pin_entry.drivers.size())
        pin_entry.pin.m_manager = nullptr;
    //Update the pin state
    pin_entry.pin.update_pin_state();
}


void PinManager::set_driver_enabled(PinDriver& drv, PinDriver::pin_index_t pin_index, bool enabled)
{
    //Find the driver entry for this driver ID
    drv_entry_t* drv_entry = m_drivers[drv.m_id];

    //Only continue if there's an actual change
    if (enabled == drv_entry->enabled_pins[pin_index]) return;
    drv_entry->enabled_pins[pin_index] = enabled;

    //A negative mux index indicates that this driver has no mux configuration at all. No point continuing.
    if (drv_entry->current_mux_index < 0) return;

    //Get the current mux configuration for this driver
    pin_id_t* pin_ids = drv_entry->mux_configs[drv_entry->current_mux_index];

    if (enabled) {
        //If enabling, add this driver to each pin in its mux config
        add_driver_to_pin(*m_pins.at(*pin_ids), drv, pin_index);
    } else {
        //If disabling, remove this driver from each pin in its mux config
        remove_driver_from_pin(*m_pins.at(*pin_ids), drv, pin_index);
    }
}


bool PinManager::driver_enabled(const PinDriver& drv, PinDriver::pin_index_t pin_index) const
{
    auto drv_entry = m_drivers.at(drv.m_id);
    return drv_entry->enabled_pins[pin_index];
}


Pin* PinManager::pin(pin_id_t pin_id) const
{
    if (m_pins.count(pin_id))
        return &(m_pins.at(pin_id)->pin);
    else
        return nullptr;
}


Wire::state_t PinManager::override_gpio(pin_id_t pin_id, const Pin::controls_t& gpio_controls)
{
    pin_entry_t* pin_entry = m_pins.at(pin_id);
    Wire::state_t s;
    for (auto [drv, pin_index] : pin_entry->drivers) {
        Pin::controls_t c = drv->override_gpio(pin_index, gpio_controls);
        s = Wire::resolve_two_states(s, controls_to_state(c));
    }
    return s;
}


void PinManager::notify_digital_state(pin_id_t pin_id, bool state)
{
    pin_entry_t* pin_entry = m_pins.at(pin_id);
    for (auto [drv, pin_index] : pin_entry->drivers) {
        drv->digital_state_changed(pin_index, state);
    }
}


void PinManager::unregister_driver(PinDriver& drv)
{
    drv_entry_t* drv_entry = m_drivers.at(drv.m_id);
    set_current_mux(drv.m_id, -1);
    m_drivers.erase(drv.m_id);
    delete drv_entry;

    drv.m_manager = nullptr;
}
