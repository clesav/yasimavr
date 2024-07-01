/*
 * sim_peripheral.h
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

#ifndef __YASIMAVR_PERIPHERAL_H__
#define __YASIMAVR_PERIPHERAL_H__

#include "sim_ioreg.h"
#include "sim_signal.h"
#include "sim_logger.h"

YASIMAVR_BEGIN_NAMESPACE

class Device;
class InterruptHandler;
class CycleTimer;
enum class SleepMode;


//=======================================================================================
/**
   \file
   \defgroup core_peripheral Peripheral base framework
   @{
 */

/**
   \name Peripheral identifier definition

   All peripherals are uniquely identified by a 32-bits integer which is actually
   composed of 4 characters.
   This section defines the identifiers for the usual peripherals.
   \sa Peripheral
   @{
*/

/** CTLID for the core: "CORE" */
#define AVR_IOCTL_CORE              chr_to_id('C', 'O', 'R', 'E')
/** CTLID for the watchdog timer: "WTDG" */
#define AVR_IOCTL_WTDG              chr_to_id('W', 'T', 'D', 'G')
/** CTLID for the interrupt controller: "INTR" */
#define AVR_IOCTL_INTR              chr_to_id('I', 'N', 'T', 'R')
/** CTLID for the sleep controller: "SLP" */
#define AVR_IOCTL_SLEEP             chr_to_id('S', 'L', 'P', '\0')
/** CTLID for the clock controller: "CLK" */
#define AVR_IOCTL_CLOCK             chr_to_id('C', 'L', 'K', '\0')
/** CTLID for the I/O port controller: "IOGx" x='A','B',... */
#define AVR_IOCTL_PORT(n)           chr_to_id('I', 'O', 'G', (n))
/** CTLID for the port mux controller */
#define AVR_IOCTL_PORTMUX           chr_to_id('I', 'O', 'M', 'X')
/** CTLID for the analog-to-digital converter: "ADCn", n=0,1,... */
#define AVR_IOCTL_ADC(n)            chr_to_id('A', 'D', 'C', (n))
/** CTLID for the analog comparator: "ACPn", n=0,1,... */
#define AVR_IOCTL_ACP(n)            chr_to_id('A', 'C', 'P', (n))
/** CTLID for the timer/counter: "TCtn", t='A','B'; n=0,1,... */
#define AVR_IOCTL_TIMER(t, n)       chr_to_id('T', 'C', (t), (n))
/** CTLID for the EEPROM controller: "EPRM" */
#define AVR_IOCTL_EEPROM            chr_to_id('E', 'P', 'R', 'M')
/** CTLID for the NVM controller: "NVM" */
#define AVR_IOCTL_NVM               chr_to_id('N', 'V', 'M', '\0')
/** CTLID for the voltage reference controller: "VREF" */
#define AVR_IOCTL_VREF              chr_to_id('V', 'R', 'E', 'F')
/** CTLID for the external interrupt controller: "EINT" */
#define AVR_IOCTL_EXTINT            chr_to_id('E', 'I', 'N', 'T')
/** CTLID for the reset controller: "RST" */
#define AVR_IOCTL_RST               chr_to_id('R', 'S', 'T', '\0')
/** CTLID for the real-time counter: "RTC" */
#define AVR_IOCTL_RTC               chr_to_id('R', 'T', 'C', '\0')
/** CTLID for the USART interface: "UAXn" */
#define AVR_IOCTL_UART(n)           chr_to_id('U', 'A', 'X', (n))
/** CTLID for the SPI interface: "SPIn" */
#define AVR_IOCTL_SPI(n)            chr_to_id('S', 'P', 'I', (n))
/** CTLID for the TWI interface: "TWIn" */
#define AVR_IOCTL_TWI(n)            chr_to_id('T', 'W', 'I', (n))

///@}

//=======================================================================================

/**
   \name Controller requests definition
   Definition of common and builtin CTLREQs
   \sa Device::ctlreq
   \sa ctlreq_data_t
   @{
 */


/**
   CTLREQ identifier type
 */
typedef int ctlreq_id_t;


/**
   Common request identifier used to obtain a pointer to a particular signal
    - data.index should contain the identifier of the signal
    - data.p is returned pointing to the signal
 */
#define AVR_CTLREQ_GET_SIGNAL       0

/**
   Request sent by the CPU to the core when a BREAK instruction is executed, no data provided.
 */
#define AVR_CTLREQ_CORE_BREAK       1

/**
   Request sent by the Sleep Controller to the core to enter a sleep mode
    - data.u contains the sleep mode enum value
 */
#define AVR_CTLREQ_CORE_SLEEP       2

/**
   Request sent by the Sleep Controller to the core to wake up from a sleep mode, no data provided
 */
#define AVR_CTLREQ_CORE_WAKEUP      3

/**
   Request sent by the Port Controller to the core when a pin shorting is detected
 */
#define AVR_CTLREQ_CORE_SHORTING    4

/**
   Request sent to the core to crash.
    - data.index is the reason code,
    - data.p is the optional reason string
 */
#define AVR_CTLREQ_CORE_CRASH       5

/**
   Request sent to the core to trigger a MCU reset.
    - data.u is the corresponding ResetFlag enum value
 */
#define AVR_CTLREQ_CORE_RESET       6

/**
   Request sent to the core to query the latest cause of reset.
    - data.u is set to the ResetFlag enum value
 */
#define AVR_CTLREQ_CORE_RESET_FLAG  7

/**
   Request sent to the core to query the pointer to a NVM block
    - data.index indicates which block with one of the AVR_NVM enum values
 */
#define AVR_CTLREQ_CORE_NVM         8

/**
   Request to halt the CPU, used during a SPM instruction.
   a non-zero data.u enables the halt, data.u == 0 disables the halt.
 */
#define AVR_CTLREQ_CORE_HALT        9


#define AVR_CTLREQ_CORE_SECTIONS    10


/**
   Request sent by the CPU to the watchdog when executing a WDR instruction, no data provided
 */
#define AVR_CTLREQ_WATCHDOG_RESET   1

/**
   Request sent by the CPU to the NVM controller when executing a SPM instruction,
   or a LPM instruction if the LPM direct mode is disabled with the core.
    - data.p points to a NVM_request_t structure filled with the instruction information
 */
#define AVR_CTLREQ_NVM_REQUEST      1

/**
   Request sent by the CPU to the Sleep Controller when executing a SLEEP instruction, no data provided
 */
#define AVR_CTLREQ_SLEEP_CALL       1

/**
   Request sent by the CPU to the Sleep Controller when executing a "RJMP .-2" instruction, no data provided
 */
#define AVR_CTLREQ_SLEEP_PSEUDO     2

/// @}


/**
 * \brief Structure used for AVR_CTLREQ_NVM_REQUEST requests.

   These structure are used when :
   - a SPM instruction is executed, or
   - flash memory is read and direct mode is disabled.

   These requests are processed by the NVM controller (if it exists) and returned with the result field set.
   This system allows to implement access control measures and self-programming features.
 */
struct NVM_request_t {
    /// Kind of request : 0:write (SPM), 1:read (LPM)
    unsigned char kind;
    /// Memory block being written/read : -1 if unknown/irrelevant, otherwise one of AVR_NVM enumeration values
    int nvm;
    /// Address to write/read (in the appropriate block address space)
    mem_addr_t addr;
    /// Value [to write to/read from] the NVM
    uint16_t data;
    /// Result of the request : >0:success, 0:ignored, <0:error/refused
    signed char result;
    /// Number of cycles to be consumed, only for write (SPM) requests and if result>=0
    unsigned short cycles;
};


/** Structure exchanged with CTL requests */
struct ctlreq_data_t {
    vardata_t data;
    long long index;
};


//=======================================================================================
/**
   \name Register field lookup
   The structure base_reg_config_t and the functions find_reg_config() are useful for
   configuration that maps a register field value to a set of parameters.
   (see the timer classes for examples)
   @{
 */

struct base_reg_config_t {
    uint64_t reg_value;
};

template<typename T>
int find_reg_config(const std::vector<T>& v, uint64_t reg_value)
{
    for (auto it = v.cbegin(); it != v.cend(); ++it) {
        const base_reg_config_t* cfg = &(*it);
        if (cfg->reg_value == reg_value)
            return it - v.cbegin();
    }
    return -1;
}

template<typename T>
const T* find_reg_config_p(const std::vector<T>& v, uint64_t reg_value)
{
    for (const T& cfg : v) {
        if (cfg.reg_value == reg_value)
            return &cfg;
    }
    return nullptr;
}

/// @}
/// @}


//=======================================================================================
/**
   \ingroup core_peripheral
   \brief Abstract class defining a framework for MCU peripherals.
 */
class AVR_CORE_PUBLIC_API Peripheral: public IO_RegHandler {

public:

    explicit Peripheral(ctl_id_t id);
    virtual ~Peripheral();

    ctl_id_t id() const;
    std::string name() const;

    virtual bool init(Device& device);

    virtual void reset();

    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data);

    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;

    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

    virtual void sleep(bool on, SleepMode mode);

    Peripheral(const Peripheral&) = delete;
    Peripheral& operator=(const Peripheral&) = delete;

protected:

    Device* device() const;

    Logger& logger();

    void add_ioreg(const regbit_t& rb, bool readonly = false);
    void add_ioreg(const regbit_compound_t& rbc, bool readonly = false);
    void add_ioreg(reg_addr_t addr, uint8_t mask = 0xFF, bool readonly = false);

    //Primary methods to access a I/O register. Note that it's not limited to those
    //for which the peripheral has registered itself to.
    uint8_t read_ioreg(reg_addr_t reg) const;
    void write_ioreg(const regbit_t& rb, uint8_t value);

    //Secondary methods for operating on I/O register values or single bits
    uint8_t read_ioreg(const regbit_t& rb) const;
    uint64_t read_ioreg(const regbit_compound_t& rbc) const;
    uint8_t read_ioreg(reg_addr_t reg, const bitmask_t& bm) const;

    bool test_ioreg(reg_addr_t reg, uint8_t bit) const;
    bool test_ioreg(reg_addr_t reg, const bitmask_t& bm) const;
    bool test_ioreg(const regbit_t& rb, uint8_t bit = 0) const;

    void write_ioreg(const regbit_compound_t& rbc, uint64_t value);
    inline void write_ioreg(reg_addr_t reg, uint8_t value);
    void write_ioreg(reg_addr_t reg, uint8_t bit, uint8_t value);
    void write_ioreg(reg_addr_t reg, const bitmask_t& bm, uint8_t value);

    void set_ioreg(const regbit_t& rb);
    void set_ioreg(const regbit_compound_t& rbc);
    void set_ioreg(reg_addr_t reg, uint8_t bit);
    void set_ioreg(reg_addr_t reg, const bitmask_t& bm);
    void set_ioreg(const regbit_t& rb, uint8_t bit);

    void clear_ioreg(const regbit_t& rb);
    void clear_ioreg(const regbit_compound_t& rbc);
    void clear_ioreg(reg_addr_t reg);
    void clear_ioreg(reg_addr_t reg, uint8_t bit);
    void clear_ioreg(reg_addr_t reg, const bitmask_t& bm);
    void clear_ioreg(const regbit_t& rb, uint8_t bit);

    bool register_interrupt(int_vect_t vector, InterruptHandler& handler) const;

    Signal* get_signal(ctl_id_t ctl_id) const;

private:

    ctl_id_t m_id;
    Device* m_device;
    Logger m_logger;

};

/// Unique identifier of the peripheral
inline ctl_id_t Peripheral::id() const
{
    return m_id;
}

/// Access to the device. It is null before init() is called.
inline Device *Peripheral::device() const
{
    return m_device;
}

inline Logger& Peripheral::logger()
{
    return m_logger;
}

inline uint8_t Peripheral::read_ioreg(const regbit_t& rb) const
{
    return rb.extract(read_ioreg(rb.addr));
}

inline uint8_t Peripheral::read_ioreg(reg_addr_t reg, const bitmask_t& bm) const
{
    return bm.extract(read_ioreg(reg));
}

inline bool Peripheral::test_ioreg(reg_addr_t reg, uint8_t bit) const
{
    return read_ioreg(regbit_t(reg, bit));
}

inline bool Peripheral::test_ioreg(reg_addr_t reg, const bitmask_t& bm) const
{
    return !!read_ioreg(regbit_t(reg, bm));
}

inline bool Peripheral::test_ioreg(const regbit_t& rb, uint8_t bit) const
{
    return !!read_ioreg(regbit_t(rb.addr, rb.bit + bit));
}

inline void Peripheral::set_ioreg(const regbit_t& rb)
{
    write_ioreg(rb, 0xFF);
}

inline void Peripheral::set_ioreg(reg_addr_t reg, uint8_t bit)
{
    set_ioreg(regbit_t(reg, bit));
}

inline void Peripheral::set_ioreg(reg_addr_t reg, const bitmask_t& bm)
{
    set_ioreg(regbit_t(reg, bm));
}

inline void Peripheral::set_ioreg(const regbit_t& rb, uint8_t bit)
{
    set_ioreg(regbit_t(rb.addr, rb.bit + bit));
}

inline void Peripheral::clear_ioreg(const regbit_t& rb)
{
    write_ioreg(rb, 0x00);
}

inline void Peripheral::clear_ioreg(const reg_addr_t reg)
{
    write_ioreg(reg, 0x00);
}

inline void Peripheral::clear_ioreg(reg_addr_t reg, uint8_t bit)
{
    clear_ioreg(regbit_t(reg, bit));
}

inline void Peripheral::clear_ioreg(reg_addr_t reg, const bitmask_t& bm)
{
    clear_ioreg(regbit_t(reg, bm));
}

inline void Peripheral::clear_ioreg(const regbit_t& rb, uint8_t bit)
{
    clear_ioreg(regbit_t(rb.addr, rb.bit + bit));
}

inline void Peripheral::write_ioreg(reg_addr_t reg, uint8_t value)
{
    write_ioreg(regbit_t(reg), value);
}

inline void Peripheral::write_ioreg(reg_addr_t reg, const bitmask_t& bm, uint8_t value)
{
    write_ioreg(regbit_t(reg, bm), value);
}

inline void Peripheral::write_ioreg(reg_addr_t reg, uint8_t bit, uint8_t value)
{
    write_ioreg(regbit_t(reg, bit), value);
}


//=======================================================================================
/**
   \brief Generic dummy peripheral.

   It does nothing but adding I/O registers.
 */

class AVR_CORE_PUBLIC_API DummyController : public Peripheral {

public:

    struct dummy_register_t {
        regbit_t reg; ///< Address of the I/O register
        uint8_t reset; ///< Reset value of the I/O register
    };

    DummyController(ctl_id_t id, const std::vector<dummy_register_t>& regs);

    virtual bool init(Device& device) override;
    virtual void reset() override;

private:

    const std::vector<dummy_register_t> m_registers;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_PERIPHERAL_H__
