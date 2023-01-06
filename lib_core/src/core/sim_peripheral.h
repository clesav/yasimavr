/*
 * sim_peripheral.h
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

#ifndef __YASIMAVR_PERIPHERAL_H__
#define __YASIMAVR_PERIPHERAL_H__

#include "sim_ioreg.h"
#include "sim_signal.h"
#include "sim_logger.h"

class AVR_Device;
class AVR_InterruptHandler;
class AVR_CycleTimer;
enum class AVR_SleepMode;


//=======================================================================================
/*
 * Peripheral identifier definition
 * All peripherals are uniquely identified by a 32-bits integer which is actually
 * composed of 4 characters.
 * This section defines the identifiers for the usual peripherals
*/

#define AVR_IOCTL_CORE              AVR_ID('C', 'O', 'R', 'E')      //Core
#define AVR_IOCTL_WTDG              AVR_ID('W', 'T', 'D', 'G')      //Watchdog
#define AVR_IOCTL_INTR              AVR_ID('I', 'N', 'T', 'R')      //Interrupt controller
#define AVR_IOCTL_SLEEP             AVR_ID('S', 'L', 'P', '\0')     //Sleep mode controller
#define AVR_IOCTL_CLOCK             AVR_ID('C', 'L', 'K', '\0')     //Clock controller
#define AVR_IOCTL_PORT(n)           AVR_ID('I', 'O', 'G', (n))      //Port controller
#define AVR_IOCTL_PORTMUX           AVR_ID('I', 'O', 'M', 'X')      //Port Mux controller
#define AVR_IOCTL_ADC(n)            AVR_ID('A', 'D', 'C', (n))      //Analog-to-digital converter
#define AVR_IOCTL_ACP(n)            AVR_ID('A', 'C', 'P', (n))      //Analog comparator
#define AVR_IOCTL_TIMER(t, n)       AVR_ID('T', 'C', (t), (n))      //Timer/counter
#define AVR_IOCTL_EEPROM            AVR_ID('E', 'P', 'R', 'M')      //EEPROM controller
#define AVR_IOCTL_NVM               AVR_ID('N', 'V', 'M', '\0')     //Non-Volative Memory controller
#define AVR_IOCTL_VREF              AVR_ID('V', 'R', 'E', 'F')      //Voltage reference controller
#define AVR_IOCTL_EXTINT            AVR_ID('E', 'I', 'N', 'T')      //External Interrupt controller
#define AVR_IOCTL_RST               AVR_ID('R', 'S', 'T', '\0')     //Reset controller
#define AVR_IOCTL_RTC               AVR_ID('R', 'T', 'C', '\0')     //Real Time Counter
#define AVR_IOCTL_UART(n)           AVR_ID('U', 'A', 'X', (n))      //UART interface
#define AVR_IOCTL_SPI(n)            AVR_ID('S', 'P', 'I', (n))      //SPI interface
#define AVR_IOCTL_TWI(n)            AVR_ID('T', 'W', 'I', (n))      //TWI interface


//=======================================================================================
/*
 * CTLREQ definitions
*/

//Common request identifier used to obtain a pointer to a particular signal
//The data.index should contain the identifier of the signal
//The data.p is returned pointing to the signal
#define AVR_CTLREQ_GET_SIGNAL       0

//Request sent by the CPU to the core when a BREAK instruction is executed, no data provided
#define AVR_CTLREQ_CORE_BREAK       1
//Request sent by the Sleep Controller to the core to enter a sleep mode
//The data.u contains the sleep mode enum value
#define AVR_CTLREQ_CORE_SLEEP       2
//Request sent by the Sleep Controller to the core to wake up from a sleep mode, no data provided
#define AVR_CTLREQ_CORE_WAKEUP      3
//Request sent by the Port Controller to the core when a pin shorting is detected
#define AVR_CTLREQ_CORE_SHORTING    4
//Request sent to the core to crash. data.index is the reason code, data.p is the optional reason string
#define AVR_CTLREQ_CORE_CRASH       5
//Request sent to the core to trigger a MCU reset. data.u is the corresponding ResetFlag enum value
#define AVR_CTLREQ_CORE_RESET       6
//Request sent to the core to query the latest cause of reset. data.u is set to the ResetFlag enum value
#define AVR_CTLREQ_CORE_RESET_FLAG  7
//Request sent to the core to query the pointer to a NVM block
//  data.index indicates which block with one of the AVR_NVM enum values
#define AVR_CTLREQ_CORE_NVM         8
//Request to halt the CPU, used during a SPM instruction.
//a non-zero data.u enables the halt, data.u == 0 disables the halt
#define AVR_CTLREQ_CORE_HALT        9

//Request sent by the CPU to the watchdog when executing a WDR instruction, no data provided
#define AVR_CTLREQ_WATCHDOG_RESET   1

//Request sent by the CPU to the NVM controller when executing a SPM instruction
//  data.p points to a NVM_request_t structure filled with the instruction information
#define AVR_CTLREQ_NVM_WRITE        1

//Structure used for AVR_CTLREQ_NVM_WRITE requests
struct NVM_request_t {
    int nvm;                //Memory block being written : -1 if unknown/irrelevant,
                            //otherwise one of AVR_NVM enumeration values
    mem_addr_t addr;        //Address to write (in the appropriate block address space)
    uint16_t data;          //Value to write to the NVM
    flash_addr_t instr;     //Write instruction address (future use for access control)
};

//Request sent by the CPU to the Sleep Controller when executing a SLEEP instruction, no data provided
#define AVR_CTLREQ_SLEEP_CALL       1
//Request sent by the CPU to the Sleep Controller when executing a "RJMP .-2" instruction, no data provided
#define AVR_CTLREQ_SLEEP_PSEUDO     2


struct ctlreq_data_t {
    vardata_t data;
    uint32_t index;
};


//=======================================================================================
/*
 * The structure base_reg_config_t and find_reg_config are useful for configuration that
 * maps a register field value to a set of parameters (see the timer classes for examples)
 */
struct base_reg_config_t {
    uint8_t reg_value;
};

template<typename T>
int find_reg_config(const std::vector<T>& v, uint8_t reg_value)
{
    for (auto it = v.cbegin(); it != v.cend(); ++it) {
        const base_reg_config_t* cfg = &(*it);
        if (cfg->reg_value == reg_value)
            return it - v.cbegin();
    }
    return -1;
}


//=======================================================================================
/*
 * Class AVR_Peripheral
 * Abstract class defining a framework for MCU peripherals
 */
class DLL_EXPORT AVR_Peripheral: public AVR_IO_RegHandler {

public:

    explicit AVR_Peripheral(uint32_t id);
    virtual ~AVR_Peripheral();

    uint32_t id() const;
    std::string name() const;

    //Callback method called when the device is initialised. This is where the peripheral can
    //allocate its I/O registers, interrupts or connect signals
    //The return boolean indicates the success of all allocations
    virtual bool init(AVR_Device& device);

    //Callback method called when the device is reset. Note that resetting I/O registers is only
    //necessary here if their reset value is not zero.
    virtual void reset();

    //Callback method called for a CTL request. The method must return true if the request has
    //been processed
    virtual bool ctlreq(uint16_t req, ctlreq_data_t* data);

    //Callback method called when the CPU is reading a I/O register allocated by this peripheral.
    //The value has not been read yet so the module can modify it before the CPU gets it.
    //'addr' is the register address in I/O space
    virtual void ioreg_read_handler(reg_addr_t addr) override;

    //Callback method called when the CPU is writing a I/O register allocated by this peripheral.
    //The value has already been written.
    //'addr' is the register address in I/O space,
    //'data' is the new register content
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

    //Callback method called when the device enters or exits a sleep mode.
    //'on' is true when entering a sleep mode, false when exiting it.
    //'mode' is one of the enum AVR_SleepMode values
    virtual void sleep(bool on, AVR_SleepMode mode);

    //Disable copy semantics
    AVR_Peripheral(const AVR_Peripheral&) = delete;
    AVR_Peripheral& operator=(const AVR_Peripheral&) = delete;

protected:

    //Access to the device. It is null before init() is called
    AVR_Device* device() const;

    AVR_Logger& logger();

    void add_ioreg(regbit_t rb, bool readonly = false);
    void add_ioreg(reg_addr_t addr, uint8_t mask = 0xFF, bool readonly = false);

    //Primary methods to access a I/O register. Note that it's not limited to those
    //for which the peripheral has registered itself to.
    uint8_t read_ioreg(reg_addr_t reg) const;
    void write_ioreg(const regbit_t rb, uint8_t value);

    //Secondary methods for operating on I/O register values or single bits
    uint8_t read_ioreg(const regbit_t rb) const;
    uint8_t read_ioreg(reg_addr_t reg, const bitmask_t bm) const;
    bool test_ioreg(reg_addr_t reg, uint8_t bit) const;
    bool test_ioreg(reg_addr_t reg, const bitmask_t bm) const;
    bool test_ioreg(const regbit_t rb, uint8_t bit = 0) const;
    void write_ioreg(reg_addr_t reg, uint8_t value);
    void write_ioreg(reg_addr_t reg, uint8_t bit, uint8_t value);
    void write_ioreg(reg_addr_t reg, const bitmask_t bm, uint8_t value);
    void set_ioreg(const regbit_t rb);
    void set_ioreg(reg_addr_t reg, uint8_t bit);
    void set_ioreg(reg_addr_t reg, const bitmask_t bm);
    void set_ioreg(const regbit_t rb, uint8_t bit);
    void clear_ioreg(const regbit_t rb);
    void clear_ioreg(reg_addr_t reg);
    void clear_ioreg(reg_addr_t reg, uint8_t bit);
    void clear_ioreg(reg_addr_t reg, const bitmask_t bm);
    void clear_ioreg(const regbit_t rb, uint8_t bit);

    //Helper function to register an interrupt vector
    bool register_interrupt(int_vect_t vector, AVR_InterruptHandler& handler) const;

    //Helper function to obtain a pointer to a signal from another peripheral
    AVR_Signal* get_signal(uint32_t ctl_id) const;

private:

    uint32_t m_id;
    AVR_Device* m_device;
    AVR_Logger m_logger;

};

inline uint32_t AVR_Peripheral::id() const
{
    return m_id;
}

inline AVR_Device *AVR_Peripheral::device() const
{
    return m_device;
}

inline AVR_Logger& AVR_Peripheral::logger()
{
    return m_logger;
}

inline uint8_t AVR_Peripheral::read_ioreg(regbit_t rb) const
{
    return rb.extract(read_ioreg(rb.addr));
}

inline uint8_t AVR_Peripheral::read_ioreg(reg_addr_t reg, const bitmask_t bm) const
{
    return bm.extract(read_ioreg(reg));
}

inline bool AVR_Peripheral::test_ioreg(reg_addr_t reg, uint8_t bit) const
{
    return read_ioreg(regbit_t(reg, bit));
}

inline bool AVR_Peripheral::test_ioreg(reg_addr_t reg, const bitmask_t bm) const
{
    return !!read_ioreg(regbit_t(reg, bm));
}

inline bool AVR_Peripheral::test_ioreg(const regbit_t rb, uint8_t bit) const
{
    return !!read_ioreg(regbit_t(rb.addr, rb.bit + bit));
}

inline void AVR_Peripheral::set_ioreg(const regbit_t rb)
{
    write_ioreg(rb, 0xFF);
}

inline void AVR_Peripheral::set_ioreg(reg_addr_t reg, uint8_t bit)
{
    set_ioreg(regbit_t(reg, bit));
}

inline void AVR_Peripheral::set_ioreg(reg_addr_t reg, const bitmask_t bm)
{
    set_ioreg(regbit_t(reg, bm));
}

inline void AVR_Peripheral::set_ioreg(const regbit_t rb, uint8_t bit)
{
    set_ioreg(regbit_t(rb.addr, rb.bit + bit));
}

inline void AVR_Peripheral::clear_ioreg(const regbit_t rb)
{
    write_ioreg(rb, 0x00);
}

inline void AVR_Peripheral::clear_ioreg(const reg_addr_t reg)
{
    write_ioreg(reg, 0x00);
}

inline void AVR_Peripheral::clear_ioreg(reg_addr_t reg, uint8_t bit)
{
    clear_ioreg(regbit_t(reg, bit));
}

inline void AVR_Peripheral::clear_ioreg(reg_addr_t reg, const bitmask_t bm)
{
    clear_ioreg(regbit_t(reg, bm));
}

inline void AVR_Peripheral::clear_ioreg(const regbit_t rb, uint8_t bit)
{
    clear_ioreg(regbit_t(rb.addr, rb.bit + bit));
}

inline void AVR_Peripheral::write_ioreg(reg_addr_t reg, uint8_t value)
{
    write_ioreg(regbit_t(reg), value);
}

inline void AVR_Peripheral::write_ioreg(reg_addr_t reg, const bitmask_t bm, uint8_t value)
{
    write_ioreg(regbit_t(reg, bm), value);
}

inline void AVR_Peripheral::write_ioreg(reg_addr_t reg, uint8_t bit, uint8_t value)
{
    write_ioreg(regbit_t(reg, bit), value);
}


//=======================================================================================
/*
 * Class AVR_DummyController
 * Class defining a dummy peripheral. It does nothing but adding I/O registers.
 */

class DLL_EXPORT AVR_DummyController : public AVR_Peripheral {

public:

    struct dummy_register_t {
        regbit_t reg;
        uint8_t reset;
    };

    AVR_DummyController(uint32_t id, const std::vector<dummy_register_t>& regs);

    virtual bool init(AVR_Device& device) override;
    virtual void reset() override;

private:

    const std::vector<dummy_register_t> m_registers;

};


#endif //__YASIMAVR_PERIPHERAL_H__
