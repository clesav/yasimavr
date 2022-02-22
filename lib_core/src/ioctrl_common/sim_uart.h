/*
 * sim_uart.h
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_IO_UART_H__
#define __YASIMAVR_IO_UART_H__

#include "../core/sim_types.h"
#include "../core/sim_device.h"
#include "../core/sim_signal.h"
#include <deque>


//=======================================================================================
/*
 * CTLREQ definitions
*/
//Request that can be used by external code to access the end point of a UART interface
//which can then be used to send & receive data via signaling both ways
//The data.p is set to the UART_EndPoint structure to connect to.
#define AVR_CTLREQ_UART_ENDPOINT		1

/*
 * Signal indexes definitions
*/
enum UART_SignalIndex {
	UART_Data_Frame,		//TX and RX : the data is a single frame in data.u
	UART_Data_String,		//RX only: the data is a c-style string in data.s
	UART_TX_Start,
	UART_TX_Complete,
	UART_RX_Start,
	UART_RX_Complete,
};


/*
 * Structure exchanged with CTLREQ_UART_ENDPOINT
*/
struct UART_EndPoint {

	AVR_Signal* tx_signal;
	AVR_SignalHook* rx_hook;

};


//=======================================================================================
/*
 * Generic class defining an universal asynchronous serial interface a.k.a. UART
 *
 * TX:
 * The TX part is composed of a FIFO, whose front slot is the shift register
 * push_tx() puts a new 8-bits frame into the FIFO and the transmission will start
 * immediately. If a TX is already in progress, the frame will wait until it can
 * be transmitted. IF the TX buffer size reached the limit, the most recently pushed
 * frames will be discarded and the collision flag will be set.
 * Frames are sent via signaling, using both UART_Data_Frame and UART_TX_Start
 * At the end of transmission, a signal UART_TX_Complete is emitted with data = 1
 * if successful or 0 if canceled mid-way by a reset.
 * On-going TX can only be canceled by a reset.
 *
 * RX:
 * The RX part is made of a FIFO with two sub-parts:
 * The front part is the actual device FIFO, from which received frames are read and popped.
 * The back part has the frames yet to be received by the device. This is
 * a convenient system that allows to send a whole string to the device in one signal, while
 * the device will still receive the characters one by one with a proper timing.
 * Disabling the RX does not prevent receiving frames. They are simply discarded when actually
 * received by the device (i.e. when moved from the back FIFO to the front FIFO)
 * Frames are received when signaled with UART_Data_Frame or UART_Data_String.
 * The signal UART_RX_Start is emitted at the start of a reception
 * The signal UART_RX_Complete are emitted at the end of a reception, with data = 1 if the frame
 * if kept or data = 0 if canceled or discarded
 */
class DLL_EXPORT AVR_IO_UART : public AVR_SignalHook {

public:

	AVR_IO_UART();
	virtual ~AVR_IO_UART();

	//Initialise the interface. the device will be used for timer related operations
	void init(AVR_CycleManager& cycle_manager, AVR_DeviceLogger& logger);

	//Reset the interface, clear the buffers and cancel any operation
	void reset();

	//Return the internal signal used for operation signaling
	AVR_Signal& signal();

	//Set the delay in clock ticks to emit or receive a frame
	//The minimum valid value is 1
	void set_frame_delay(cycle_count_t delay);

	//Set the TX buffer size, including the TX shift register. 0 means unlimited
	void set_tx_buffer_limit(unsigned int limit);

	//Push a 8-bits frame to be emitted by the interface. If no TX is already
	//ongoing, it will be started immediately
	void push_tx(uint8_t frame);

	//Cancel all pending TX but let the current one finish, if any.
	void cancel_tx_pending();

	//Return the no of frames waiting in the buffer to be emitted.
	unsigned int tx_pending() const;

	//Return/clear the TX collision flag
	bool has_tx_collision() const;
	void clear_tx_collision();

	//Set the RX buffer size, including the RX shift register, 0 means unlimited
	void set_rx_buffer_limit(unsigned int limit);

	//Enable/disable the reception. If disabled, the RX buffer is flushed.
	void set_rx_enabled(bool enabled);

	//Return the no of received frames in the buffer and not popped
	unsigned int rx_available() const;

	//Pop a frame from the RX buffer. Returns 0 if no frame available.
	uint8_t pop_rx();

	//Return/clear the RX overflow flag
	bool has_rx_overflow() const;
	void clear_rx_overflow();

protected:

	//Implementation of the AVR_Signal::Hook interface to receive frames
	//from the outside
	virtual void raised(const signal_data_t& data, uint16_t id) override;

private:

	class RxTimer;
	class TxTimer;
	friend class RxTimer;
	friend class TxTimer;

	AVR_CycleManager* m_cycle_manager;
	AVR_DeviceLogger* m_logger;

	AVR_Signal m_signal;

	//Frame delay in clock cycles
	cycle_count_t m_delay;
	//TX FIFO buffer. The front is the TX shift register
	std::deque<uint8_t> m_tx_buffer;
	//Size limit for the TX FIFO, including the shift register
	unsigned int m_tx_limit;
	//Collision flag
	bool m_tx_collision;
	//Cycle timer to simulate the delay to emit a frame
	TxTimer* m_tx_timer;

	//Enable/disable flag for RX
	bool m_rx_enabled;
	//RX FIFO buffer, it has two parts, delimited by m_rx_count
	//the front part is the device actual FIFO, the back part
	//is the buffer for frames yet to be received
	std::deque<uint8_t> m_rx_buffer;
	//It is actually the no of frames in the device FIFO,
	//hence delimiting the FIFO two sub-parts
	unsigned int m_rx_count;
	//Size limit for the device part of the RX FIFO
	//The back part of the FIFO is not limited
	unsigned int m_rx_limit;
	//RX overflow flag
	bool m_rx_overflow;
	//Cycle timer to simulate the delay to receive a frame
	RxTimer* m_rx_timer;

	void add_rx_frame(uint8_t frame);
	void start_rx();

	inline bool tx_in_progress() {
		return m_tx_buffer.size() > 0;
	}

	inline bool rx_in_progress() {
		return m_rx_count < m_rx_buffer.size();
	}

	cycle_count_t rx_timer_next(cycle_count_t when);
	cycle_count_t tx_timer_next(cycle_count_t when);

};

inline AVR_Signal& AVR_IO_UART::signal()
{
	return m_signal;
}

inline unsigned int AVR_IO_UART::rx_available() const
{
	return m_rx_count;
}

inline void AVR_IO_UART::set_frame_delay(cycle_count_t delay)
{
	m_delay = delay ? delay : 1;
}

inline unsigned int AVR_IO_UART::tx_pending() const
{
	return m_tx_buffer.size() ? (m_tx_buffer.size() - 1) : 0;
}

inline bool AVR_IO_UART::has_tx_collision() const
{
	return m_tx_collision;
}

inline void AVR_IO_UART::clear_tx_collision()
{
	m_tx_collision = false;
}

inline bool AVR_IO_UART::has_rx_overflow() const
{
	return m_rx_overflow;
}

inline void AVR_IO_UART::clear_rx_overflow()
{
	m_rx_overflow = false;
}

#endif //__YASIMAVR_IO_UART_H__
