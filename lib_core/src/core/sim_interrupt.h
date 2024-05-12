/*
 * sim_interrupt.h
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

#ifndef __YASIMAVR_INTERRUPT_H__
#define __YASIMAVR_INTERRUPT_H__

#include "sim_peripheral.h"
#include "sim_types.h"
#include "sim_signal.h"

YASIMAVR_BEGIN_NAMESPACE

class InterruptHandler;

//=======================================================================================
/*
 * CTLREQ definitions
*/

//AVR_CTLREQ_GET_SIGNAL :
//    The signal returned is raised when an interrupt is raised
//    the index is set to the vector index
//    the value 'u' is set to the interrupt state (IntrState_Raised)

//Request sent to by any peripheral to register an interrupt vector
//The index shall be the interrupt vector index
//The value 'p' shall be the corresponding InterruptHandler object
//Notes:
//   . a vector can only be registered once
//   . the vector 0 cannot be registered
#define AVR_CTLREQ_INTR_REGISTER    1

//Request sent to raise or clear artificially any interrupt
//The index shall be the interrupt vector index
//The value 'u' shall be 1 for raising the interrupt, 0 for clearing it.
#define AVR_CTLREQ_INTR_RAISE       2

#define AVR_INTERRUPT_NONE          -1


//=======================================================================================
/**
   \brief Generic interrupt controller

   It manages an interrupt vector table that the CPU can access to know if a interrupt
   routine should be executed.
   Each interrupt vector may be allocated by a interrupt handler which
   controls the raise (or cancellation) of the interrupt.

   The arbitration of priorities between vectors is left to concrete sub-classes.

   \sa AVR_InterruptHandler
*/
class AVR_CORE_PUBLIC_API InterruptController : public Peripheral {

    friend class InterruptHandler;

public:

    enum SignalId {
        /// Signal ID for indicating that the state of an interrupt has changed. index is the vector index.
        Signal_StateChange
    };

    enum State {
        ///The interrupt is raised
        State_Raised          = 0x01,
        ///The interrupt is cancelled
        State_Cancelled       = 0x10,
        ///The interrupt is acknowledged by the CPU and it's about to jump to the corresponding vector
        State_Acknowledged    = 0x20,
        ///The CPU returned from the interrupt routine
        State_Returned        = 0x30,
        ///The interrupt is raised after leaving a sleep mode where it was masked
        State_RaisedFromSleep = 0x41,
        ///The interrupt is reset because the MCU is reset
        State_Reset           = 0x50
    };

    /**
       Structure returned by the interrupt controller to the CPU containing the information
       of the interrupt to process.
     */
    struct IRQ_t {
        ///Vector index
        int_vect_t vector;
        ///Address (in bytes) of the interrupt vector
        flash_addr_t address;
        ///Non-maskable (by GIE) indicator flag
        bool nmi;
    };

    static constexpr IRQ_t NO_INTERRUPT = { AVR_INTERRUPT_NONE, 0, false };

    //===== Constructor/destructor =====
    explicit InterruptController(unsigned int vector_count);

    //===== Override of IO_CTL virtual methods =====
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void sleep(bool on, SleepMode mode) override;

    //===== Interface API for the CPU =====
    bool cpu_has_irq() const;
    IRQ_t cpu_get_irq() const;
    void cpu_ack_irq();
    virtual void cpu_reti();

protected:

    //Helper methods to access the vector table, for concrete implementing sub-classes
    bool interrupt_raised(int_vect_t vector) const;
    int_vect_t intr_count() const;
    void set_interrupt_raised(int_vect_t vector, bool raised);

    virtual void cpu_ack_irq(int_vect_t vector);

    /**
       Abstract method indicating which vector should be executed next.
       Architecture specific behaviors can be implemented here to take
       into account priority arbitration between vectors.

       \note implementations should assume the GIE flag is set.

       \return IRQ to be executed next or NO_INTERRUPT
    */
    virtual IRQ_t get_next_irq() const = 0;

    void update_irq();

private:

    //===== Structure holding data on the vector table =====
    struct interrupt_t {
        bool used = false;
        bool raised = false;
        InterruptHandler* handler = nullptr;
    };

    //Interrupt vector table
    std::vector<interrupt_t> m_interrupts;
    //Variable holding the vector to be executed next
    IRQ_t m_irq;
    //Signal raised with changes of interrupt state
    Signal m_signal;

    //private API used by the interrupt handlers
    void raise_interrupt(int_vect_t vector);
    void cancel_interrupt(int_vect_t vector);
    void disconnect_handler(InterruptHandler* handler);

};

/**
   Used by the CPU to do a quick test whether an interrupt is raised.

   \return true if there is an IRQ raised.
*/
inline bool InterruptController::cpu_has_irq() const
{
    return m_irq.vector > AVR_INTERRUPT_NONE;
}

/**
   Used by the CPU to interrogate the controller whether an interrupt is raised.
   \n If a valid vector is returned and the Global Interrupt Enable flag is set,
   the CPU initiates a jump to the corresponding routine.

   \return a vector index if there is an IRQ raised, AVR_INTERRUPT_NONE if not.
*/
inline InterruptController::IRQ_t InterruptController::cpu_get_irq() const
{
    if (m_irq.vector > AVR_INTERRUPT_NONE)
        return m_irq;
    else
        return NO_INTERRUPT;
}

///Interrupt table size getter
inline int_vect_t InterruptController::intr_count() const
{
    return m_interrupts.size();
}

///Interrupt state getter
inline bool InterruptController::interrupt_raised(int_vect_t vector) const
{
    return m_interrupts[vector].raised;
}


//=======================================================================================
/**
   \brief Abstract interface to a interrupt controller.

   It allows to raise (or cancel) an single interrupt.
   The same handler can be used for several interrupts.
   \sa AVR_InterruptController
*/
class AVR_CORE_PUBLIC_API InterruptHandler {

    friend class InterruptController;

public:

    InterruptHandler();
    virtual ~InterruptHandler();

    //Controlling method for raising (or cancelling) interrupts
    //It can actually raise any interrupt so long as it has been
    //registered with the controller
    void raise_interrupt(int_vect_t vector) const;
    void cancel_interrupt(int_vect_t vector) const;
    virtual void interrupt_ack_handler(int_vect_t vector);

    //Disable copy semantics
    InterruptHandler(const InterruptHandler&) = delete;
    InterruptHandler& operator=(const InterruptHandler&) = delete;

private:

    InterruptController* m_intctl;

};


//=======================================================================================
/**
   \brief Generic helper to manage an Interrupt Flag/Enable in a I/O register.

   The flag is made of one or several bits of a I/O register, along with corresponding
   enable bit(s).

   The interrupt is raised if and only if at least one flag bit and its
   corresponding enable bit are set.

   The flag can be configured to clear-on-ack.
   If enabled, the flag will be cleared when the interrupt is ACK'ed by the CPU.
   If disabled, the flag will be unchanged by a CPU ACK'ed. The effect is to
   keep the interrupt raised until the flag is cleared directly in the register.
*/
class AVR_CORE_PUBLIC_API InterruptFlag : public InterruptHandler {

public:

    explicit InterruptFlag(bool clear_on_ack = false);
    InterruptFlag(const InterruptFlag& other);

    bool init(Device& device, const regbit_t& rb_enable, const regbit_t& rb_flag, int_vect_t vector);

    void set_clear_on_ack(bool clear_on_ack);

    int update_from_ioreg();

    bool set_flag(uint8_t mask = 0xFF);
    bool clear_flag(uint8_t mask = 0xFF);

    bool raised() const;

    //Override to clear the flag on ACK if enabled
    virtual void interrupt_ack_handler(int_vect_t vector) override;

private:

    bool m_clr_on_ack;
    regbit_t m_rb_enable;
    regbit_t m_rb_flag;
    int_vect_t m_vector;
    bool m_raised;

    IO_Register* m_flag_reg;
    IO_Register* m_enable_reg;

    bool flag_raised() const;

};

/// Returns the raised state of the interrupt flag
inline bool InterruptFlag::raised() const
{
    return m_raised;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_INTERRUPT_H__
