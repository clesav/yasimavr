/*
 * peripheral.sip
 *
 *  Copyright 2022-2024 Clement Savergne <csavergne@yahoo.com>

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

const ctl_id_t IOCTL_CORE       = AVR_IOCTL_CORE;
const ctl_id_t IOCTL_WTDG       = AVR_IOCTL_WTDG;
const ctl_id_t IOCTL_INTR       = AVR_IOCTL_INTR;
const ctl_id_t IOCTL_SLEEP      = AVR_IOCTL_SLEEP;
const ctl_id_t IOCTL_CLOCK      = AVR_IOCTL_CLOCK;
const ctl_id_t IOCTL_PORTMUX    = AVR_IOCTL_PORTMUX;
const ctl_id_t IOCTL_EEPROM     = AVR_IOCTL_EEPROM;
const ctl_id_t IOCTL_NVM        = AVR_IOCTL_NVM;
const ctl_id_t IOCTL_VREF       = AVR_IOCTL_VREF;
const ctl_id_t IOCTL_EXTINT     = AVR_IOCTL_EXTINT;
const ctl_id_t IOCTL_RST        = AVR_IOCTL_RST;
const ctl_id_t IOCTL_RTC        = AVR_IOCTL_RTC;

ctl_id_t IOCTL_PORT(char n)
{
    return AVR_IOCTL_PORT(n);
}

ctl_id_t IOCTL_ADC(char n)
{
    return AVR_IOCTL_ADC(n);
}

ctl_id_t IOCTL_ACP(char n)
{
    return AVR_IOCTL_ACP(n);
}

ctl_id_t IOCTL_TIMER(char t, char n)
{
    return AVR_IOCTL_TIMER(t, n);
}

ctl_id_t IOCTL_UART(char n)
{
    return AVR_IOCTL_UART(n);
}

ctl_id_t IOCTL_SPI(char n)
{
    return AVR_IOCTL_SPI(n);
}

ctl_id_t IOCTL_TWI(char n)
{
    return AVR_IOCTL_TWI(n);
}


const ctlreq_id_t CTLREQ_GET_SIGNAL       = AVR_CTLREQ_GET_SIGNAL;
const ctlreq_id_t CTLREQ_CORE_BREAK       = AVR_CTLREQ_CORE_BREAK;
const ctlreq_id_t CTLREQ_CORE_SLEEP       = AVR_CTLREQ_CORE_SLEEP;
const ctlreq_id_t CTLREQ_CORE_WAKEUP      = AVR_CTLREQ_CORE_WAKEUP;
const ctlreq_id_t CTLREQ_CORE_SHORTING    = AVR_CTLREQ_CORE_SHORTING;
const ctlreq_id_t CTLREQ_CORE_CRASH       = AVR_CTLREQ_CORE_CRASH;
const ctlreq_id_t CTLREQ_CORE_RESET       = AVR_CTLREQ_CORE_RESET;
const ctlreq_id_t CTLREQ_CORE_RESET_FLAG  = AVR_CTLREQ_CORE_RESET_FLAG;
const ctlreq_id_t CTLREQ_CORE_NVM         = AVR_CTLREQ_CORE_NVM;
const ctlreq_id_t CTLREQ_CORE_HALT        = AVR_CTLREQ_CORE_HALT;
const ctlreq_id_t CTLREQ_CORE_SECTIONS    = AVR_CTLREQ_CORE_SECTIONS;
const ctlreq_id_t CTLREQ_WATCHDOG_RESET   = AVR_CTLREQ_WATCHDOG_RESET;
const ctlreq_id_t CTLREQ_NVM_REQUEST      = AVR_CTLREQ_NVM_REQUEST;
const ctlreq_id_t CTLREQ_SLEEP_CALL       = AVR_CTLREQ_SLEEP_CALL;
const ctlreq_id_t CTLREQ_SLEEP_PSEUDO     = AVR_CTLREQ_SLEEP_PSEUDO;

%End


//=======================================================================================

const ctl_id_t IOCTL_CORE;
const ctl_id_t IOCTL_WTDG;
const ctl_id_t IOCTL_INTR;
const ctl_id_t IOCTL_SLEEP;
const ctl_id_t IOCTL_CLOCK;
const ctl_id_t IOCTL_PORTMUX;
const ctl_id_t IOCTL_EEPROM;
const ctl_id_t IOCTL_NVM;
const ctl_id_t IOCTL_VREF;
const ctl_id_t IOCTL_EXTINT;
const ctl_id_t IOCTL_RST;
const ctl_id_t IOCTL_RTC;

ctl_id_t IOCTL_PORT(char);
ctl_id_t IOCTL_ADC(char);
ctl_id_t IOCTL_ACP(char);
ctl_id_t IOCTL_TIMER(char, char);
ctl_id_t IOCTL_UART(char);
ctl_id_t IOCTL_SPI(char);
ctl_id_t IOCTL_TWI(char);


typedef int ctlreq_id_t;

const ctlreq_id_t CTLREQ_GET_SIGNAL;
const ctlreq_id_t CTLREQ_CORE_BREAK;
const ctlreq_id_t CTLREQ_CORE_SLEEP;
const ctlreq_id_t CTLREQ_CORE_WAKEUP;
const ctlreq_id_t CTLREQ_CORE_SHORTING;
const ctlreq_id_t CTLREQ_CORE_CRASH;
const ctlreq_id_t CTLREQ_CORE_RESET;
const ctlreq_id_t CTLREQ_CORE_RESET_FLAG;
const ctlreq_id_t CTLREQ_CORE_NVM;
const ctlreq_id_t CTLREQ_CORE_HALT;
const ctlreq_id_t CTLREQ_CORE_SECTIONS;
const ctlreq_id_t CTLREQ_WATCHDOG_RESET;
const ctlreq_id_t CTLREQ_NVM_REQUEST;
const ctlreq_id_t CTLREQ_SLEEP_CALL;
const ctlreq_id_t CTLREQ_SLEEP_PSEUDO;


struct NVM_request_t {
%TypeHeaderCode
#include "core/sim_peripheral.h"
%End

    unsigned char kind /PyInt/;
    int nvm;
    mem_addr_t addr;
    uint16_t data;
    signed char result /PyInt/;
    unsigned short cycles;
};


struct ctlreq_data_t {
%TypeHeaderCode
#include "core/sim_peripheral.h"
%End

    vardata_t data;
    long long index;

};


struct base_reg_config_t {
%TypeHeaderCode
#include "core/sim_peripheral.h"
%End

    uint64_t reg_value;
};


class Peripheral : public IO_RegHandler /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_peripheral.h"
%End

public:

    Peripheral(ctl_id_t);

    ctl_id_t id() const;
    std::string name() const /Factory/;

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void sleep(bool on, SleepMode mode);

protected:

    Device* device() const;

    Logger& logger();

    void add_ioreg(const regbit_t&, bool = false);
    void add_ioreg(const regbit_compound_t&, bool = false);
    void add_ioreg(reg_addr_t, uint8_t = 0xFF, bool = false);

    uint8_t read_ioreg(reg_addr_t) const;
    void write_ioreg(const regbit_t&, uint8_t);

    uint8_t read_ioreg(const regbit_t&) const;
    uint64_t read_ioreg(const regbit_compound_t&) const;
    uint8_t read_ioreg(reg_addr_t, const bitmask_t&) const;

    bool test_ioreg(reg_addr_t, uint8_t) const;
    bool test_ioreg(reg_addr_t, const bitmask_t&) const;
    bool test_ioreg(const regbit_t&, uint8_t = 0) const;

    void write_ioreg(const regbit_compound_t&, uint64_t);
    void write_ioreg(reg_addr_t, uint8_t);
    void write_ioreg(reg_addr_t, uint8_t, uint8_t);
    void write_ioreg(reg_addr_t, const bitmask_t&, uint8_t);

    void set_ioreg(const regbit_t&);
    void set_ioreg(const regbit_compound_t&);
    void set_ioreg(reg_addr_t, uint8_t);
    void set_ioreg(reg_addr_t, const bitmask_t&);
    void set_ioreg(const regbit_t&, uint8_t);

    void clear_ioreg(const regbit_t&);
    void clear_ioreg(const regbit_compound_t&);
    void clear_ioreg(reg_addr_t);
    void clear_ioreg(reg_addr_t, uint8_t);
    void clear_ioreg(reg_addr_t, const bitmask_t&);
    void clear_ioreg(const regbit_t&, uint8_t);

    bool register_interrupt(int_vect_t, InterruptHandler&) const;

    Signal* get_signal(ctl_id_t) const;

};


class DummyController : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_peripheral.h"
%End

public:

    struct dummy_register_t {
        regbit_t reg;
        uint8_t reset;
    };

    DummyController(ctl_id_t, const std::vector<DummyController::dummy_register_t>&);

    virtual bool init(Device&);
    virtual void reset();

};
