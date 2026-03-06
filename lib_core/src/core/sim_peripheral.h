/*
 * sim_peripheral.h
 *
 *  Copyright 2022-2026 Clement Savergne <csavergne@yahoo.com>

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
constexpr sim_id_t AVR_IOCTL_CORE     = "CORE";
/** CTLID for the watchdog timer: "WDT" */
constexpr sim_id_t AVR_IOCTL_WDT      = "WDT";
/** CTLID for the interrupt controller: "CPUINT" */
constexpr sim_id_t AVR_IOCTL_INTR     = "CPUINT";
/** CTLID for the sleep controller: "SLPCTRL" */
constexpr sim_id_t AVR_IOCTL_SLEEP    = "SLPCTRL";
/** CTLID for the clock controller: "CLKCTRL" */
constexpr sim_id_t AVR_IOCTL_CLOCK    = "CLKCTRL";
/** CTLID for the I/O port controller: "PORTx" x='A','B',... */
constexpr sim_id_t AVR_IOCTL_PORT(char c) { return sim_id_t("PORT") + c; }
/** CTLID for the port mux controller */
constexpr sim_id_t AVR_IOCTL_PORTMUX  = "PORTMUX";
/** CTLID for the analog-to-digital converter: "ADCn", n=0,1,... */
constexpr sim_id_t AVR_IOCTL_ADC(char c) { return sim_id_t("ADC") + c; }
/** CTLID for the analog comparator: "ACPn", n=0,1,... */
constexpr sim_id_t AVR_IOCTL_ACP(char c) { return sim_id_t("ACP") + c; }
/** CTLID for the timer/counter: "TCtn", t='A','B'; n=0,1,... */
constexpr sim_id_t AVR_IOCTL_TIMER(char t, char c) { return (sim_id_t("TC") + t) + c; }
/** CTLID for the EEPROM controller: "EEPROM" */
constexpr sim_id_t AVR_IOCTL_EEPROM   = "EEPROM";
/** CTLID for the NVM controller: "NVMCTRL" */
constexpr sim_id_t AVR_IOCTL_NVM      = "NVMCTRL";
/** CTLID for the voltage reference controller: "VREF" */
constexpr sim_id_t AVR_IOCTL_VREF     = "VREF";
/** CTLID for the external interrupt controller: "EINT" */
constexpr sim_id_t AVR_IOCTL_EXTINT   = "EXTINT";
/** CTLID for the reset controller: "RSTCTRL" */
constexpr sim_id_t AVR_IOCTL_RST      = "RSTCTRL";
/** CTLID for the real-time counter: "RTC" */
constexpr sim_id_t AVR_IOCTL_RTC      = "RTC";
/** CTLID for the USART interface: "USARTn" */
constexpr sim_id_t AVR_IOCTL_UART(char c) { return sim_id_t("USART") + c; }
/** CTLID for the SPI interface: "SPIn" */
constexpr sim_id_t AVR_IOCTL_SPI(char c) { return sim_id_t("SPI") + c; }
/** CTLID for the TWI interface: "TWIn" */
constexpr sim_id_t AVR_IOCTL_TWI(char c) { return sim_id_t("TWI") + c; }

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
   Base value for peripheral-specific requests.
   The range 0-255 is reserved for generic requests
 */
#define AVR_CTLREQ_BASE             0x100

/**
   Common request identifier used to obtain a pointer to a particular signal
    - data.index should contain the identifier of the signal
    - data.p is returned pointing to the signal
 */
#define AVR_CTLREQ_GET_SIGNAL       0

/**
   Request sent by the CPU to the core when a BREAK instruction is executed, no data provided.
 */
#define AVR_CTLREQ_CORE_BREAK       (AVR_CTLREQ_BASE + 1)

/**
   Request sent by the Sleep Controller to the core to enter a sleep mode
    - data.u contains the sleep mode enum value
 */
#define AVR_CTLREQ_CORE_SLEEP       (AVR_CTLREQ_BASE + 2)

/**
   Request sent by the Sleep Controller to the core to wake up from a sleep mode, no data provided
 */
#define AVR_CTLREQ_CORE_WAKEUP      (AVR_CTLREQ_BASE + 3)

/**
   Request sent by the Port Controller to the core when a pin shorting is detected
 */
#define AVR_CTLREQ_CORE_SHORTING    (AVR_CTLREQ_BASE + 4)

/**
   Request sent to the core to crash.
    - data.index is the reason code,
    - data.p is the optional reason string
 */
#define AVR_CTLREQ_CORE_CRASH       (AVR_CTLREQ_BASE + 5)

/**
   Request sent to the core to trigger a MCU reset.
    - data.u is the corresponding ResetFlag enum value
 */
#define AVR_CTLREQ_CORE_RESET       (AVR_CTLREQ_BASE + 6)

/**
   Request sent to the core to query the latest cause of reset.
    - data.u is set to the ResetFlag enum value
 */
#define AVR_CTLREQ_CORE_RESET_FLAG  (AVR_CTLREQ_BASE + 7)

/**
   Request sent to the core to query the pointer to a NVM block
    - data.index indicates which block with one of the AVR_NVM enum values
 */
#define AVR_CTLREQ_CORE_NVM         (AVR_CTLREQ_BASE + 8)

/**
   Request to halt the CPU, used during a SPM instruction.
   a non-zero data.u enables the halt, data.u == 0 disables the halt.
 */
#define AVR_CTLREQ_CORE_HALT        (AVR_CTLREQ_BASE + 9)

/**
   Request to get the section manager.
   data.p is returned pointing to the instance of MemorySectionManager.
 */
#define AVR_CTLREQ_CORE_SECTIONS    (AVR_CTLREQ_BASE + 10)

/**
   Request sent by the CPU to the watchdog when executing a WDR instruction, no data provided
 */
#define AVR_CTLREQ_WATCHDOG_RESET   (AVR_CTLREQ_BASE + 1)

/**
   Request sent by the CPU to the NVM controller when executing a SPM instruction,
   or a LPM instruction if the LPM direct mode is disabled with the core.
    - data.p points to a NVM_request_t structure filled with the instruction information
 */
#define AVR_CTLREQ_NVM_REQUEST      (AVR_CTLREQ_BASE + 1)

/**
   Request sent by the CPU to the Sleep Controller when executing a SLEEP instruction, no data provided
 */
#define AVR_CTLREQ_SLEEP_CALL       (AVR_CTLREQ_BASE + 1)

/**
   Request sent by the CPU to the Sleep Controller when executing a "RJMP .-2" instruction, no data provided
 */
#define AVR_CTLREQ_SLEEP_PSEUDO     (AVR_CTLREQ_BASE + 2)

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
class AVR_CORE_PUBLIC_API Peripheral: public IORegHandler {

public:

    explicit Peripheral(ctl_id_t id);
    virtual ~Peripheral();

    ctl_id_t id() const;
    std::string name() const;

    virtual bool init(Device& device);

    virtual void reset();

    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data);

    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;

    virtual uint8_t ioreg_peek_handler(reg_addr_t addr, uint8_t value) override;

    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

    virtual void sleep(bool on, SleepMode mode);

    Peripheral(const Peripheral&) = delete;
    Peripheral& operator=(const Peripheral&) = delete;

protected:

    Device* device() const;

    Logger& logger();

    void add_ioreg(const regmask_t& rm, IORegister::BitMode bitmode = IORegister::RW);
    void add_ioreg(const regbit_compound_t& rbc, IORegister::BitMode bitmode = IORegister::RW);
    void add_ioreg(reg_addr_t addr, IORegister::BitMode bitmode = IORegister::RW);
    void add_ioreg(reg_addr_t addr, bitmask_t mask, IORegister::BitMode bitmode = IORegister::RW);

    uint8_t read_ioreg(const regbit_t& rb) const;
    uint64_t read_ioreg(const regbit_compound_t& rbc) const;
    uint8_t read_ioreg(reg_addr_t addr) const;
    uint8_t read_ioreg(reg_addr_t addr, const bitspec_t& bs) const;

    bool test_ioreg(const regbit_t& rb) const;
    bool test_ioreg(const regbit_compound_t& rbc) const;
    bool test_ioreg(reg_addr_t addr, const bitspec_t& bs) const;
    bool test_ioreg(reg_addr_t addr, bitmask_t bm = 0xFF) const;

    void write_ioreg(reg_addr_t addr, bitmask_t bm, uint8_t value);
    void write_ioreg(const regbit_t& rb, uint8_t value);
    void write_ioreg(const regbit_compound_t& rbc, uint64_t value);
    void write_ioreg(reg_addr_t addr, uint8_t value);
    void write_ioreg(reg_addr_t addr, const bitspec_t& bs, uint8_t value);

    void set_ioreg(reg_addr_t addr, bitmask_t bm = 0xFF);
    void set_ioreg(const regbit_t& rb);
    void set_ioreg(const regbit_compound_t& rbc);
    void set_ioreg(reg_addr_t addr, const bitspec_t& bs);

    void clear_ioreg(reg_addr_t addr, bitmask_t bm = 0xFF);
    void clear_ioreg(const regbit_t& rb);
    void clear_ioreg(const regbit_compound_t& rbc);
    void clear_ioreg(reg_addr_t addr, const bitspec_t& bs);

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

inline void Peripheral::add_ioreg(const regmask_t& rm, IORegister::BitMode bitmode)
{
    add_ioreg(rm.addr, rm.mask, bitmode);
}

inline void Peripheral::add_ioreg(reg_addr_t addr, IORegister::BitMode bitmode)
{
    add_ioreg(addr, bitmask_t(0xFF), bitmode);
}

inline uint8_t Peripheral::read_ioreg(const regbit_t& rb) const
{
    return rb.extract(read_ioreg(rb.addr));
}

inline uint8_t Peripheral::read_ioreg(reg_addr_t addr, const bitspec_t& bs) const
{
    return bs.extract(read_ioreg(addr));
}

inline bool Peripheral::test_ioreg(const regbit_t& rb) const
{
    return !!read_ioreg(rb);
}

inline bool Peripheral::test_ioreg(const regbit_compound_t& rbc) const
{
    return !!read_ioreg(rbc);
}

inline bool Peripheral::test_ioreg(reg_addr_t addr, const bitspec_t& bs) const
{
    return !!read_ioreg(addr, bs);
}

inline bool Peripheral::test_ioreg(reg_addr_t addr, bitmask_t bm) const
{
    return !!(bm & read_ioreg(addr));
}

inline void Peripheral::write_ioreg(const regbit_t& rb, uint8_t value)
{
    write_ioreg(rb.addr, (bitmask_t) rb, rb.shift_and_mask(value));
}

inline void Peripheral::write_ioreg(reg_addr_t addr, uint8_t value)
{
    write_ioreg(addr, bitmask_t(0xFF), value);
}

inline void Peripheral::write_ioreg(reg_addr_t addr, const bitspec_t& bs, uint8_t value)
{
    write_ioreg(addr, (bitmask_t) bs, bs.shift_and_mask(value));
}

inline void Peripheral::set_ioreg(reg_addr_t addr, bitmask_t bm)
{
    write_ioreg(addr, bm, 0xFF);
}

inline void Peripheral::set_ioreg(const regbit_t& rb)
{
    write_ioreg(rb, 0xFF);
}

inline void Peripheral::set_ioreg(reg_addr_t addr, const bitspec_t& bs)
{
    write_ioreg(addr, bs, 0xFF);
}

inline void Peripheral::clear_ioreg(reg_addr_t addr, bitmask_t bm)
{
    write_ioreg(addr, bm, 0x00);
}

inline void Peripheral::clear_ioreg(const regbit_t& rb)
{
    write_ioreg(rb, 0x00);
}

inline void Peripheral::clear_ioreg(reg_addr_t addr, const bitspec_t& bs)
{
    write_ioreg(addr, bs, 0x00);
}


//=======================================================================================
/**
   \brief Generic dummy peripheral.

   It does nothing but adding I/O registers.
 */

class AVR_CORE_PUBLIC_API DummyController : public Peripheral {

public:

    struct dummy_register_t {
        regmask_t reg; ///< Address and mask of the I/O register
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
