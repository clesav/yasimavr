/*
 * peripheral.sip
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


%ModuleCode

#include "core/sim_peripheral.h"

const uint32_t IOCTL_CORE       = AVR_IOCTL_CORE;
const uint32_t IOCTL_WTDG       = AVR_IOCTL_WTDG;
const uint32_t IOCTL_INTR       = AVR_IOCTL_INTR;
const uint32_t IOCTL_SLEEP      = AVR_IOCTL_SLEEP;
const uint32_t IOCTL_CLOCK      = AVR_IOCTL_CLOCK;
const uint32_t IOCTL_PORTMUX    = AVR_IOCTL_PORTMUX;
const uint32_t IOCTL_EEPROM     = AVR_IOCTL_EEPROM;
const uint32_t IOCTL_NVM        = AVR_IOCTL_NVM;
const uint32_t IOCTL_VREF       = AVR_IOCTL_VREF;
const uint32_t IOCTL_EXTINT     = AVR_IOCTL_EXTINT;
const uint32_t IOCTL_RST        = AVR_IOCTL_RST;
const uint32_t IOCTL_RTC        = AVR_IOCTL_RTC;

uint32_t IOCTL_PORT(char n)
{
    return AVR_IOCTL_PORT(n);
}

uint32_t IOCTL_ADC(char n)
{
    return AVR_IOCTL_ADC(n);
}

uint32_t IOCTL_ACP(char n)
{
    return AVR_IOCTL_ACP(n);
}

uint32_t IOCTL_TIMER(char t, char n)
{
    return AVR_IOCTL_TIMER(t, n);
}

uint32_t IOCTL_UART(char n)
{
    return AVR_IOCTL_UART(n);
}

uint32_t IOCTL_SPI(char n)
{
    return AVR_IOCTL_SPI(n);
}

uint32_t IOCTL_TWI(char n)
{
    return AVR_IOCTL_TWI(n);
}


const uint16_t CTLREQ_GET_SIGNAL       = AVR_CTLREQ_GET_SIGNAL;
const uint16_t CTLREQ_CORE_BREAK       = AVR_CTLREQ_CORE_BREAK;
const uint16_t CTLREQ_CORE_SLEEP       = AVR_CTLREQ_CORE_SLEEP;
const uint16_t CTLREQ_CORE_WAKEUP      = AVR_CTLREQ_CORE_WAKEUP;
const uint16_t CTLREQ_CORE_SHORTING    = AVR_CTLREQ_CORE_SHORTING;
const uint16_t CTLREQ_CORE_CRASH       = AVR_CTLREQ_CORE_CRASH;
const uint16_t CTLREQ_CORE_RESET       = AVR_CTLREQ_CORE_RESET;
const uint16_t CTLREQ_CORE_RESET_FLAG  = AVR_CTLREQ_CORE_RESET_FLAG;
const uint16_t CTLREQ_CORE_NVM         = AVR_CTLREQ_CORE_NVM;
const uint16_t CTLREQ_CORE_HALT        = AVR_CTLREQ_CORE_HALT;
const uint16_t CTLREQ_WATCHDOG_RESET   = AVR_CTLREQ_WATCHDOG_RESET;
const uint16_t CTLREQ_NVM_WRITE        = AVR_CTLREQ_NVM_WRITE;
const uint16_t CTLREQ_SLEEP_CALL       = AVR_CTLREQ_SLEEP_CALL;
const uint16_t CTLREQ_SLEEP_PSEUDO     = AVR_CTLREQ_SLEEP_PSEUDO;

%End

const uint32_t IOCTL_CORE;
const uint32_t IOCTL_WTDG;
const uint32_t IOCTL_INTR;
const uint32_t IOCTL_SLEEP;
const uint32_t IOCTL_CLOCK;
const uint32_t IOCTL_PORTMUX;
const uint32_t IOCTL_EEPROM;
const uint32_t IOCTL_NVM;
const uint32_t IOCTL_VREF;
const uint32_t IOCTL_EXTINT;
const uint32_t IOCTL_RST;
const uint32_t IOCTL_RTC;

uint32_t IOCTL_PORT(char);
uint32_t IOCTL_ADC(char);
uint32_t IOCTL_ACP(char);
uint32_t IOCTL_TIMER(char, char);
uint32_t IOCTL_UART(char);
uint32_t IOCTL_SPI(char);
uint32_t IOCTL_TWI(char);

const uint16_t CTLREQ_GET_SIGNAL;
const uint16_t CTLREQ_CORE_BREAK;
const uint16_t CTLREQ_CORE_SLEEP;
const uint16_t CTLREQ_CORE_WAKEUP;
const uint16_t CTLREQ_CORE_SHORTING;
const uint16_t CTLREQ_CORE_CRASH;
const uint16_t CTLREQ_CORE_RESET;
const uint16_t CTLREQ_CORE_RESET_FLAG;
const uint16_t CTLREQ_CORE_NVM;
const uint16_t CTLREQ_CORE_HALT;
const uint16_t CTLREQ_WATCHDOG_RESET;
const uint16_t CTLREQ_NVM_WRITE;
const uint16_t CTLREQ_SLEEP_CALL;
const uint16_t CTLREQ_SLEEP_PSEUDO;

struct NVM_request_t {
%TypeHeaderCode
#include "core/sim_peripheral.h"
%End

    int nvm;
    mem_addr_t addr;
    uint16_t data;
    flash_addr_t instr;

};


//=======================================================================================

struct ctlreq_data_t {
%TypeHeaderCode
#include "core/sim_peripheral.h"
%End

    vardata_t data;
    uint32_t index;

};


struct base_reg_config_t {
%TypeHeaderCode
#include "core/sim_peripheral.h"
%End

    uint8_t reg_value;
};


class AVR_Peripheral : public AVR_IO_RegHandler {
%TypeHeaderCode
#include "core/sim_peripheral.h"
%End

public:

    AVR_Peripheral(uint32_t);
    virtual ~AVR_Peripheral();

    uint32_t id() const;
    const char* name() const;
    %MethodCode
        sipRes = sipCpp->name().c_str();
    %End

    virtual bool init(AVR_Device&);
    virtual void reset();
    virtual bool ctlreq(uint16_t, ctlreq_data_t*);
    virtual void ioreg_read_handler(reg_addr_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void sleep(bool on, AVR_SleepMode mode);

protected:

    AVR_Device* device() const;

    AVR_Logger& logger();

    void add_ioreg(regbit_t, bool = false);
    void add_ioreg(reg_addr_t, uint8_t = 0xFF, bool = false);

    uint8_t read_ioreg(reg_addr_t) const;
    void write_ioreg(const regbit_t, uint8_t);

    uint8_t read_ioreg(const regbit_t) const;
    uint8_t read_ioreg(reg_addr_t, const bitmask_t) const;
    bool test_ioreg(reg_addr_t, uint8_t) const;
    bool test_ioreg(reg_addr_t, const bitmask_t) const;
    bool test_ioreg(const regbit_t, uint8_t = 0) const;
    void write_ioreg(reg_addr_t, uint8_t);
    void write_ioreg(reg_addr_t, uint8_t, uint8_t);
    void write_ioreg(reg_addr_t, const bitmask_t, uint8_t);
    void set_ioreg(const regbit_t);
    void set_ioreg(reg_addr_t, uint8_t);
    void set_ioreg(reg_addr_t, const bitmask_t);
    void set_ioreg(const regbit_t, uint8_t);
    void clear_ioreg(const regbit_t);
    void clear_ioreg(reg_addr_t, uint8_t);
    void clear_ioreg(reg_addr_t, const bitmask_t);
    void clear_ioreg(const regbit_t, uint8_t);

    bool register_interrupt(int_vect_t, AVR_InterruptHandler*) const;

    AVR_Signal* get_signal(uint32_t) const;

};


class AVR_DummyController : public AVR_Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_peripheral.h"
%End

public:

    struct dummy_register_t {
        regbit_t reg;
        uint8_t reset;
    };

    AVR_DummyController(uint32_t, std::vector<AVR_DummyController::dummy_register_t>);

    virtual bool init(AVR_Device&);
    virtual void reset();

};
