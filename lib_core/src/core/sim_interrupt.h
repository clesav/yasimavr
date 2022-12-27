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

class AVR_InterruptHandler;

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
//The value 'p' shall be the corresponding AVR_InterruptHandler object
//Notes:
//   . a vector can only be registered once
//   . the vector 0 cannot be registered
#define AVR_CTLREQ_INTR_REGISTER    1

#define AVR_INTERRUPT_NONE          -1


//=======================================================================================
/*
 * Class AVR_InterruptController
 * Peripheral implementation of a generic interrupt controller
 * It manages an interrupt vector table that the CPU can access to know if a interrupt
 * routine should be executed.
 * The arbitration of priorities between vectors is left to concrete sub-classes.
*/
class DLL_EXPORT AVR_InterruptController : public AVR_Peripheral {

    friend class AVR_InterruptHandler;

public:

    enum SignalId {
        Signal_Raised,
        Signal_Cancelled,
        Signal_Acknowledged,
        Signal_Returned,
    };

    //===== Constructor/destructor =====
    //Construct the controller with the given vector table size
    explicit AVR_InterruptController(unsigned int size);

    //===== Override of IO_CTL virtual methods =====
    virtual void reset() override;
    virtual bool ctlreq(uint16_t req, ctlreq_data_t* data) override;
    virtual void sleep(bool on, AVR_SleepMode mode) override;

    //===== Interface API for the CPU =====
    //Returns a vector index if there is an IRQ raised, AVR_INTERRUPT_NONE if not.
    //If a valid vector is returned and the Global Interrupt Enable flag is set,
    //the CPU initiates a jump to the corresponding routine.
    int_vect_t cpu_get_irq() const;
    //Called to ACK the IRQ obtained with cpu_get_irq()
    void cpu_ack_irq();
    //Called when returning from an interrupt service routine
    virtual void cpu_reti();

protected:

    enum InterruptState {
        IntrState_Unused,
        IntrState_Idle,
        IntrState_Raised,
    };

    //Helper methods to access the vector table, for concrete implementing sub-classes
    InterruptState interrupt_state(unsigned int vector) const;
    unsigned int intr_count() const;
    void set_interrupt_state(int_vect_t vector, InterruptState state);

    //Called by cpu_ack_irq with the vector obtained by cpu_get_irq()
    //By default it sets the interrupt state back to Idle and calls the handler
    virtual void cpu_ack_irq(int_vect_t vector);
    //Abstract method to indicate if a vector should be executed next.
    //This should not take into account the value of the GIE flag
    virtual int_vect_t get_next_irq() const = 0;

    void update_irq();

private:

    //===== Structure holding data on the vector table =====
    struct AVR_Interrupt {

        InterruptState state;
        AVR_InterruptHandler* handler;

        AVR_Interrupt();

    };

    //Interrupt vector table
    std::vector<AVR_Interrupt> m_interrupts;
    //Variable holding the vector to be executed next
    int_vect_t m_irq_vector;
    AVR_Signal m_signal;

    //Interface for the interrupt handlers
    void raise_interrupt(int_vect_t vector);
    void cancel_interrupt(int_vect_t vector);
    void disconnect_handler(AVR_InterruptHandler* handler);
};

inline int_vect_t AVR_InterruptController::cpu_get_irq() const
{
    return m_irq_vector;
}

inline unsigned int AVR_InterruptController::intr_count() const
{
    return m_interrupts.size();
}

inline AVR_InterruptController::InterruptState AVR_InterruptController::interrupt_state(unsigned int vector) const
{
    return m_interrupts[vector].state;
}


//=======================================================================================
/*
 * Class AVR_InterruptHandler
 * Abstract interface to a interrupt controller
 * It allows to raise (or cancel) an interrupt
 * The same handler can be used for several interrupts.
*/
class DLL_EXPORT AVR_InterruptHandler {

    friend class AVR_InterruptController;

public:

    AVR_InterruptHandler();
    virtual ~AVR_InterruptHandler();

    //Controlling method for raising (or cancelling) interrupts
    //It can actually raise any interrupt so long as it has been
    //registered with the controller
    void raise_interrupt(int_vect_t vector) const;
    void cancel_interrupt(int_vect_t vector) const;
    //Callback method called when a vector has been ACK'ed by the CPU
    //(i.e. it is about to execute the corresponding vector table entry)
    //The default does nothing
    virtual void interrupt_ack_handler(int_vect_t vector);

private:

    AVR_InterruptController* m_intctl;

};


//=======================================================================================
/*
 * Class AVR_InterruptFlag
 * Generic helper to manage an Interrupt Flag/Enable in a I/O register
*/
class DLL_EXPORT AVR_InterruptFlag : public AVR_InterruptHandler {

public:

    explicit AVR_InterruptFlag(bool clear_on_ack);

    //Init allocates the resources required. Returns true on success
    bool init(AVR_Device& device, const regbit_t& rb_enable, const regbit_t& rb_flag, int_vect_t vector);

    int update_from_ioreg();

    bool set_flag(uint8_t mask = 0xFF);
    bool clear_flag(uint8_t mask = 0xFF);

    bool raised() const;

    //bool trigger(uint8_t mask = 0xFF);
    //bool trigger_if_set(uint8_t mask = 0xFF);

    //bool cancel(uint8_t mask = 0xFF);
    //bool cancel_if_clear(uint8_t mask = 0xFF);

    //Override to clear the flag on ACK if enabled
    virtual void interrupt_ack_handler(int_vect_t vector) override;

private:

    const bool m_clr_on_ack;
    regbit_t m_rb_enable;
    regbit_t m_rb_flag;
    int_vect_t m_vector;
    bool m_raised;

    AVR_IO_Register* m_flag_reg;
    AVR_IO_Register* m_enable_reg;

    bool flag_raised() const;

};

inline bool AVR_InterruptFlag::raised() const
{
    return m_raised;
}

#endif //__YASIMAVR_INTERRUPT_H__
