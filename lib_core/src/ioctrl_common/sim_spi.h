/*
 * sim_spi.h
 *
 *  Copyright 2021-2025 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_SPI_H__
#define __YASIMAVR_SPI_H__

#include "../core/sim_types.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \file
   \defgroup api_spi Serial Peripheral Interface framework
   @{
 */

/**
   \name Controller requests definition for SPI
   @{
 */

/**
   Request to transfer a byte bypassing . A byte is directly
   dropped into the RX buffer and a byte popped from the TX buffer of the SPI interface
   with bypassing the actual line shifting. This is meant for debugging purposes.
    - In argument, data is a 8-bits frame to be pushed to the RX buffer.
    - data is returned set to a 8-bits frame popped from the TX buffer.
 */
#define AVR_CTLREQ_SPI_TRANSFER         (AVR_CTLREQ_BASE + 1)

/// @}
/// @}


//=======================================================================================

namespace SPI {

enum SerialMode {
    Mode0 = 0,
    Mode1,
    Mode2,
    Mode3,
};


enum BitOrder {
    MSBFirst,
    LSBFirst
};


enum Line {
    Clock = 0,
    MISO,
    MOSI,
    Select,
};


class AVR_CORE_PUBLIC_API EndPoint {

public:

    EndPoint();
    virtual ~EndPoint() = default;

    void set_serial_mode(SerialMode mode);
    SerialMode serial_mode() const;

    void set_bit_order(BitOrder order);
    BitOrder bit_order() const;

    void set_shift_data(uint8_t frame);
    uint8_t shift_data() const;

    bool complete_frame() const;

protected:

    void set_active(bool active);
    bool active() const;

    virtual void frame_completed();
    virtual void write_data_output(bool level) = 0;
    virtual bool read_data_input() = 0;

    void set_shift_clock(bool state);
    bool shift_clock() const;

private:

    SerialMode m_serial_mode;
    BitOrder m_bit_order;
    int m_step;
    bool m_active;
    uint8_t m_shifter;
    bool m_sampler;
    bool m_shift_clock;

    void update_sdo();
    void shift_and_sample();

};

inline SerialMode EndPoint::serial_mode() const
{
    return m_serial_mode;
}

inline BitOrder EndPoint::bit_order() const
{
    return m_bit_order;
}

inline uint8_t EndPoint::shift_data() const
{
    return m_shifter;
}

inline bool EndPoint::active() const
{
    return m_active;
}

inline bool EndPoint::shift_clock() const
{
    return m_shift_clock;
}

inline bool EndPoint::complete_frame() const
{
    return !m_step;
}


}; //namespace SPI


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_SPI_H__
