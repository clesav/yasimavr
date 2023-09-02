/*
 * sim_signal.h
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

#ifndef __YASIMAVR_SIGNAL_H__
#define __YASIMAVR_SIGNAL_H__

#include "sim_types.h"
#include <stdint.h>
#include <vector>
#include <unordered_map>

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/*
 * Generic structure for the data passed on when raising a signal
 */

struct signal_data_t {

    int sigid;
    long long index;
    vardata_t data;

};


//=======================================================================================
/*
 * Generic hook class to connect to a signal in order to receive notifications
 * To be used, reimplement raised() to use the data from the signal
 */
class Signal;

class AVR_CORE_PUBLIC_API SignalHook {

public:

    SignalHook() = default;
    //Copy constructor: to ensure the connection with signals is consistent
    SignalHook(const SignalHook&);
    //No move constructor
    SignalHook(const SignalHook&&) = delete;
    //Destructor: severs all connections with signals
    virtual ~SignalHook();

    virtual void raised(const signal_data_t& sigdata, int hooktag) = 0;

    //Copy assignment : copy all signal connections
    SignalHook& operator=(const SignalHook&);
    //No move assignment
    SignalHook& operator=(const SignalHook&&) = delete;

private:

    friend class Signal;

    std::vector<Signal*> m_signals;

};


//=======================================================================================
/*
 * Signal is a means for point-to-point communication and notification
 * across the simulator.
 * It is similar in use to simavr's IRQs
*/
class AVR_CORE_PUBLIC_API Signal {

public:

    Signal();
    //Copy constructor
    Signal(const Signal& other);
    //No move constructor
    Signal(const Signal&&) = delete;
    //Destructor: severs all connections with hooks
    virtual ~Signal();

    ////The hooktag is an arbitrary value that only has a meaning
    //for the hook and is passed though by the signal when calling
    //the hook's "raised()". It can be useful when a single hook
    //connects to several signals, in order to differentiate which
    //one the raise comes from.
    void connect(SignalHook& hook, int hooktag = 0);
    void disconnect(SignalHook& hook);

    //Raise a signal
    virtual void raise(const signal_data_t& sigdata);
    void raise(int sigid = 0, const vardata_t& v = vardata_t(), long long index = 0);

    //Copy assignment
    Signal& operator=(const Signal&);
    //No move assignment
    Signal& operator=(const Signal&&) = delete;

private:

    friend class SignalHook;

    //Flag used to avoid nested raises
    bool m_busy;

    struct hook_slot_t {
        SignalHook* hook;
        int tag;
    };

    std::vector<hook_slot_t> m_hooks;

    int hook_index(const SignalHook& hook) const;
    int signal_index(const SignalHook& hook) const;

};


//=======================================================================================

class AVR_CORE_PUBLIC_API DataSignal : public Signal {

public:

    vardata_t data(int sigid, long long index = 0) const;
    bool has_data(int sigid, long long index = 0) const;
    void set_data(int sigid, const vardata_t& v, long long index = 0);

    void clear();

    virtual void raise(const signal_data_t& sigdata) override;
    using Signal::raise;

private:

    struct key_t {
        int sigid;
        long long index;
        bool operator==(const key_t& other) const;
    };

    struct keyhash_t {
        size_t operator()(const key_t& k) const;
    };

    std::unordered_map<key_t, vardata_t, keyhash_t> m_data;

};


//=======================================================================================

class AVR_CORE_PUBLIC_API DataSignalMux : public SignalHook {

public:

    DataSignalMux();

    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

    size_t add_mux();
    size_t add_mux(DataSignal& signal);
    size_t add_mux(DataSignal& signal, int sigid_filt);
    size_t add_mux(DataSignal& signal, int sigid_filt, long long ix_filt);

    DataSignal& signal();

    void set_selection(size_t index);
    size_t selected_index() const;
    bool connected() const;

private:

    struct mux_item_t {
        DataSignal* signal;
        int sigid_filt;
        long long index_filt;
        uint8_t filt_mask;
        vardata_t data;

        bool match(const signal_data_t& sigdata) const;
    };

    std::vector<mux_item_t> m_items;
    DataSignal m_signal;
    size_t m_sel_index;

    size_t add_mux(mux_item_t& item);

};

inline DataSignal& DataSignalMux::signal()
{
    return m_signal;
}

inline size_t DataSignalMux::selected_index() const
{
    return m_sel_index;
}

inline bool DataSignalMux::connected() const
{
    return (m_sel_index < m_items.size()) ? !!m_items[m_sel_index].signal : false;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_SIGNAL_H__
