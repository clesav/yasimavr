/*
 * sim_device.cpp
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

#include "sim_device.h"
#include "sim_firmware.h"
#include "sim_sleep.h"
#include "../ioctrl_common/sim_vref.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

/**
   Construct a device model
 */
Device::Device(Core& core, const DeviceConfiguration& config)
:m_core(core)
,m_config(config)
,m_options(Option_InfiniteLoopDetect)
,m_state(State_Limbo)
,m_frequency(0)
,m_sleep_mode(SleepMode::Active)
,m_debugger(nullptr)
,m_logger(chr_to_id('D', 'E', 'V', 0), m_log_handler)
,m_cycle_manager(nullptr)
,m_reset_flags(0)
{
    //Allocate the pin array
    for (unsigned int i = 0; i < config.pins.size(); ++i) {
        pin_id_t id = str_to_id(config.pins[i]);
        Pin* pin = new Pin(id);
        m_pins[id] = pin;
    }
}

/**
   Destroy the device model and all the attached peripheral.
 */
Device::~Device()
{
    m_state = State_Destroying;
    erase_peripherals();
}


void Device::erase_peripherals()
{
    //Destroys all the peripherals, last attached first destroyed.
    for (auto per_it = m_peripherals.rbegin(); per_it != m_peripherals.rend(); ++per_it) {
        Peripheral* per = *per_it;
        delete per;
    }
    m_peripherals.clear();

    //Destroys the device pins.
    for (auto it = m_pins.begin(); it != m_pins.end(); ++it) {
        Pin* pin = it->second;
        delete pin;
    }
    m_pins.clear();
}


//=======================================================================================

/**
   Set or clear a device option.
   \sa Device::Option
 */
void Device::set_option(Option option, bool value)
{
    if (value)
        m_options |= option;
    else
        m_options &= ~option;
}

/**
   Returns whether a device option is set.
   \sa Device::Option
 */
bool Device::test_option(Option option) const
{
    return m_options & option;
}


//=======================================================================================

/**
   Initialise a device. This must be called once before the simulation is started.
   This function allows all peripherals to allocate resources and connect signals.
   \param cycle_manager used for scheduling timers during the device model execution.
   \return true if the initialisation has succeeded, false if it failed.
 */
bool Device::init(CycleManager& cycle_manager)
{
    if (m_state != State_Limbo)
        return false;

    m_cycle_manager = &cycle_manager;

    m_log_handler.init(cycle_manager);

    m_logger.dbg("Initialisation of %s core", m_config.name);
    if (!m_core.init(*this)) {
        m_logger.err("Initialisation of %s core failed.", m_config.name);
        return false;
    }

    for (auto per : m_peripherals) {
        const char* per_name = id_to_str(per->id()).c_str();
        m_logger.dbg("Initialisation of peripheral '%s'", per_name);
        if (!per->init(*this)) {
            m_logger.err("Initialisation of peripheral '%s' of %s failed.",
                         per_name,
                         m_config.name);
            return false;
        }
    }

    m_state = State_Ready;
    reset();

    m_logger.dbg("Initialisation of device '%s' complete", m_config.name);

    return true;
}

/**
   Simulates a MCU reset.
   \param reset_flag combination of ResetFlag enum values, indicating the source of the reset signal
 */
void Device::reset(int reset_flag)
{
    m_logger.dbg("Device reset");

    m_reset_flags |= reset_flag;

    m_core.reset();

    for (auto per : m_peripherals)
        per->reset();

    m_core.m_pc = reset_vector();

    if (m_state >= State_Running && m_state < State_Done)
        m_state = (m_reset_flags & Reset_Halt) ? State_Halted : State_Running;

    m_reset_flags = 0;
    m_sleep_mode = SleepMode::Active;
}

/**
   Returns the reset vector address in bytes. The default implementation returns 0x0000.
 */
flash_addr_t Device::reset_vector()
{
    return 0x0000;
}

/**
   Load a firmware into the device non-volatile memories.
   \return true if the load succeeded, false if it failed
   \sa Firmware
 */
bool Device::load_firmware(const Firmware& firmware)
{
    if (m_state != State_Ready) {
        m_logger.err("Firmware load: Device not ready");
        return false;
    }

    if (!program(firmware))
        return false;

    //Stores the frequency. Compulsory.
    if (!firmware.frequency) {
        m_logger.err("Firmware load: MCU frequency not defined");
        return false;
    }
    m_frequency = firmware.frequency;

    //Send the power supply voltage from the firmware to the VREF controller (if it exists)
    bool analog_ok = false;
    if (firmware.vcc > 0.0) {
        ctlreq_data_t reqdata = { .data = firmware.vcc, .index = VREF::Source_VCC, };
        analog_ok = ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_SET, &reqdata);
        if (analog_ok) {
            //Send the analog voltage reference from the firmware to the VREF controller
            reqdata.index = VREF::Source_AREF;
            reqdata.data = firmware.aref;
            ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_SET, &reqdata);
        } else {
            m_logger.err("Firmware load: Unable to set VCC, analog features are unusable.");
        }
    } else {
        m_logger.dbg("Firmware load: VCC not defined in the firmware, analog features are unusable.");
    }

    //Set the console register
    m_core.set_console_register(firmware.console_register);

    m_state = State_Running;

    return true;
}

/**
   Implementation of the programming of the non-volatile memories of the device.
   The basic implementation only loads the flash and the fuses, the rest
   is the responsibility of architecture-specific implementations.
   \return true if the load succeeded, false if it failed
   \sa Firmware
 */
bool Device::program(const Firmware& firmware)
{
    if (!firmware.has_memory(Firmware::Area_Flash)) {
        m_logger.err("Firmware load: No program to load");
        return false;
    }
    else if (firmware.load_memory(Firmware::Area_Flash, m_core.m_flash)) {
        m_logger.dbg("Loaded %d bytes of flash", firmware.memory_size(Firmware::Area_Flash));
    } else {
        m_logger.err("Firmware load: The flash does not fit");
        return false;
    }

    if (firmware.has_memory(Firmware::Area_Fuses)) {
        if (firmware.load_memory(Firmware::Area_Fuses, m_core.m_fuses)) {
            m_logger.dbg("Firmware load: fuses loaded");
        } else {
            m_logger.err("Firmware load: Error programming the fuses");
            return false;
        }
    }

    return true;
}

/**
   Execute one instruction cycle.
   \return the number of clock cycle consumed by the instruction, or 0
   if something wrong happened.
 */
cycle_count_t Device::exec_cycle()
{
    if (!(m_state & 0x0F)) return 0;

    cycle_count_t cycle_delta;
    if (m_state == State_Running)
        cycle_delta = m_core.exec_cycle();
    else
        cycle_delta = 1;

    if (m_state == State_Reset)
        reset();

    return cycle_delta;
}


//=======================================================================================
//Management of I/O peripherals

/**
   Attach a peripheral to the device. The device takes ownership of the peripheral
   and will destroy it upon destruction.
   \param ctl peripheral to attach
 */
void Device::attach_peripheral(Peripheral& ctl)
{
    if (ctl.id() == AVR_IOCTL_INTR)
        m_core.m_intrctl = reinterpret_cast<InterruptController*>(&ctl);

    m_peripherals.push_back(&ctl);
}

/**
   Process a peripheral request. This is the mechanism used to interrogate peripherals or
   the device itself.
   \param id identifier of the peripheral to interrogate
   \param req request identifier, specific to each peripheral
   \param reqdata data structure of the request
   \return true if the request could be processed, false otherwise.
 */
bool Device::ctlreq(ctl_id_t id, ctlreq_id_t req, ctlreq_data_t* reqdata)
{
    if (id == AVR_IOCTL_CORE) {
        return core_ctlreq(req, reqdata);
    } else {
        for (auto per : m_peripherals) {
            if (id == per->id()) {
                return per->ctlreq(req, reqdata);
            }
        }

        m_logger.wng("Sending request but peripheral %s not found", id_to_str(id).c_str());
        return false;
    }
}

Peripheral* Device::find_peripheral(const char* name)
{
    return find_peripheral(str_to_id(name));
}

/**
   Finds a peripheral given its identifier
   the device itself.
   \return the peripheral if found or nullptr
 */
Peripheral* Device::find_peripheral(ctl_id_t id)
{
    for (auto per : m_peripherals) {
        if (per->id() == id)
            return per;
    }
    return nullptr;
}

/**
   Adds a handler to a I/O register.
   \param addr address of the I/O register, in I/O address space
   \param handler handler to add
   \param ro_mask optional read-only bit mask. By default = 0x00 (all bits are R/W)

   \note The register is allocated if it does not exist yet.
   All bits of the register are marked as used and bits marked as '1' in ro_mask are
   marked as read-only. This is OR'ed with any pre-defined read-only mask.
 */
void Device::add_ioreg_handler(reg_addr_t addr, IO_RegHandler& handler, uint8_t ro_mask)
{
    if (addr != R_SREG && addr >= 0) {
        m_logger.dbg("Registering handler for I/O 0x%04X", addr);
        IO_Register* reg = m_core.get_ioreg(addr);
        reg->set_handler(handler, 0xFF, ro_mask);
    }
}

/**
   Adds a handler to a part of a I/O register.
   \param rb address/mask of the bits I/O register, in I/O address space
   \param handler handler to add
   \param readonly

   \note The register is allocated if it does not exist yet.
   All bits of the regbit mask are marked as used and also marked as read-only if 'readonly' is true
 */
void Device::add_ioreg_handler(const regbit_t& rb, IO_RegHandler& handler, bool readonly)
{
    if (rb.addr != R_SREG && rb.valid()) {
        m_logger.dbg("Registering handler for I/O 0x%04X", rb.addr);
        IO_Register* reg = m_core.get_ioreg(rb.addr);
        reg->set_handler(handler, rb.mask, readonly ? rb.mask : 0x00);
    }
}

/**
   Callback for processing the requests to the core.
   \sa ctlreq()
 */
bool Device::core_ctlreq(ctlreq_id_t req, ctlreq_data_t* reqdata)
{
    if (req == AVR_CTLREQ_CORE_BREAK) {
        if (m_core.m_debug_probe) {
            m_logger.wng("Device break at PC=%04x", m_core.m_pc);
            m_state = State_Break;
        }
        return true;
    }

    else if (req == AVR_CTLREQ_CORE_SLEEP) {
        if (test_option(Option_InfiniteLoopDetect) && !m_core.m_sreg[SREG_I]) {
            //The device cannot get out of sleep or infinite loop if GIE=0.
            //If the detect option is enabled, we exit the sim loop.
            if ((SleepMode) reqdata->data.as_uint() == SleepMode::Pseudo)
                m_logger.wng("Device in infinite loop with GIE=0, stopping.");
            else
                m_logger.wng("Device going to sleep with GIE=0, stopping.");

            m_logger.wng("End of program at PC = 0x%04x", m_core.m_pc);
            m_state = State_Done;

        } else {

            m_state = State_Sleeping;
            m_sleep_mode = (SleepMode) reqdata->data.as_int();

            if (m_sleep_mode == SleepMode::Pseudo)
                m_logger.dbg("Device going to pseudo sleep");
            else
                m_logger.dbg("Device going to sleep mode %s", SleepModeName(m_sleep_mode));

            for (auto ioctl : m_peripherals)
                ioctl->sleep(true, m_sleep_mode);
        }

        return true;
    }

    else if (req == AVR_CTLREQ_CORE_WAKEUP) {
        if (m_state == State_Sleeping) {
            m_logger.dbg("Device waking up");
            for (auto per : m_peripherals)
                per->sleep(false, m_sleep_mode);

            m_state = State_Running;
        }

        m_sleep_mode = SleepMode::Active;

        return true;
    }

    else if (req == AVR_CTLREQ_CORE_SHORTING) {
        pin_id_t pin_id = reqdata->data.as_uint();
        m_logger.err("Pin %s shorted", id_to_str(pin_id).c_str());
        if (m_options & Option_ResetOnPinShorting) {
            m_reset_flags |= Reset_PowerOn;
            m_state = State_Reset;
        } else {
            m_state = State_Crashed;
        }
        return true;
    }

    else if (req == AVR_CTLREQ_CORE_CRASH) {
        m_logger.err("CPU crash, reason=%d", reqdata->index);
        m_logger.wng("End of program at PC = 0x%04x", m_core.m_pc);
        m_state = State_Crashed;
        return true;
    }

    else if (req == AVR_CTLREQ_CORE_RESET) {
        m_reset_flags |= reqdata->data.as_uint();
        m_state = State_Reset;
        m_logger.wng("MCU reset triggered, Flags = 0x%08x", m_reset_flags);
        return true;
    }

    else if (req == AVR_CTLREQ_CORE_RESET_FLAG) {
        reqdata->data = m_reset_flags;
        return true;
    }

    else if (req == AVR_CTLREQ_CORE_NVM) {
        reqdata->data = (void*) nullptr;
        if (reqdata->index == Core::NVM_Flash)
            reqdata->data = &(m_core.m_flash);
        else if (reqdata->index == Core::NVM_Fuses)
            reqdata->data = &(m_core.m_fuses);
        return true;
    }

    else if (req == AVR_CTLREQ_CORE_HALT) {
        if (reqdata->data.as_uint()) {
            if (m_state == State_Running || m_state == State_Sleeping) {
                m_state = State_Halted;
                m_logger.dbg("Device halted");
            }
        } else {
            if (m_state == State_Halted) {
                m_state = (m_sleep_mode == SleepMode::Active) ? State_Running : State_Sleeping;
                m_logger.dbg("Device resuming from halt");
            }
        }
        return true;
    }

    return false;
}


//=======================================================================================
//Management of device pins

/**
   Find a device pin with the given name
   \return the pin if found, or nullptr
 */
Pin* Device::find_pin(const char* name)
{
    return find_pin(str_to_id(name));
}

/**
   Find a device pin with the given identifier
   \return the pin if found, or nullptr
 */
Pin* Device::find_pin(pin_id_t id)
{
    auto search = m_pins.find(id);
    if (search == m_pins.end())
        return nullptr;
    else
        return search->second;
}


//=======================================================================================
//Management of the crashes

/**
   Set the device to the crashed state
   \param reason one of the CRASH_XXX codes
   \param text message of the crash
 */
void Device::crash(uint16_t reason, const char* text)
{
    m_logger.err("MCU crash, reason (code=%d) : %s", reason, text);
    m_logger.wng("End of program at PC = 0x%04x", m_core.m_pc);
    m_state = State_Crashed;
}
