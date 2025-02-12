/*
 * sim_spi.cpp
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

#include "sim_spi.h"

YASIMAVR_USING_NAMESPACE

using namespace SPI;

//=======================================================================================

EndPoint::EndPoint()
:m_serial_mode(Mode0)
,m_bit_order(MSBFirst)
,m_step(0)
,m_active(false)
,m_shifter(0)
,m_sampler(0)
,m_shift_clock(false)
{}


void EndPoint::set_serial_mode(SerialMode mode)
{
    m_serial_mode = mode;
}


void EndPoint::set_bit_order(BitOrder bitorder)
{
    m_bit_order = bitorder;
    update_sdo();
}


void EndPoint::set_shift_data(uint8_t frame)
{
    m_shifter = frame;
    update_sdo();
}


void EndPoint::set_active(bool active)
{
    if (active && !m_active)
        m_step = 0;

    m_active = active;
}


void EndPoint::update_sdo()
{
    bool bit = (m_bit_order == MSBFirst) ? (m_shifter & 0x80) : (m_shifter & 0x01);
    write_data_output(bit);
}


void EndPoint::shift_and_sample()
{
    bool sampler = read_data_input();
    if (m_bit_order == MSBFirst)
        m_shifter = ((m_shifter << 1) & 0xFE) | (sampler ? 0x01 : 0);
    else
        m_shifter = ((m_shifter >> 1) & 0x7F) | (sampler ? 0x80 : 0);
}


void EndPoint::set_shift_clock(bool state)
{
    if (state == m_shift_clock) return;
    m_shift_clock = state;

    if (!m_active) return;

    bool cpol = m_serial_mode & 0x02;
    if (cpol)
        state = !state;

    bool cpha = m_serial_mode & 0x01;
    if (cpha) {
        if (state)
            update_sdo();
        else
            shift_and_sample();
    } else {
        if (state) {
            shift_and_sample();
        } else {
            if (m_step < 15)
                update_sdo();
        }
    }

    if (m_step < 15) {
        m_step++;
    } else {
        m_step = 0;
        frame_completed();
    }
}


void EndPoint::frame_completed() {}
