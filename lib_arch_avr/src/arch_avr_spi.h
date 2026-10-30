/*
 * arch_avr_spi.h
 *
 *  Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_AVR_SPI_H__
#define __YASIMAVR_AVR_SPI_H__

#include "arch_avr_globals.h"
#include "core/sim_interrupt.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \brief Configuration structure for ArchAVR_SPI
 */
struct ArchAVR_SPIConfig {

    /// Address for the data register
    reg_addr_t reg_data;
    /// Regbit for the SPE register bit
    regbit_t rb_enable;
    /// Regbit for the SPIE register bit
    regbit_t rb_int_enable;
    /// Regbit for the SPIF register flag
    regbit_t rb_int_flag;
    /// Regbit for the master/slave mode (MSTR) register bit
    regbit_t rb_mode;
    /// Regbit for the CPOL register bit
    regbit_t rb_cpol;
    /// Regbit for the CPHA register bit
    regbit_t rb_cpha;
    /// Regbit for the DORD register bit
    regbit_t rb_dord;
    /// Regbit for the clock rate selection register field
    regbit_t rb_clock;
    /// Regbit for the x2 clock (SPI2X) register bit
    regbit_t rb_clock2x;
    /// Regbit for the WCOL register flag
    regbit_t rb_wcol;
    /// Index of the %SPI interrupt vector
    int_vect_t iv_spi;

};

/**
   \brief Implementation of a Serial %Peripheral Interface interface for AVR series

   __CTLREQs supported__ :
   - AVR_CTLREQ_SPI_TRANSFER
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_SPI : public Peripheral {

public:

    ArchAVR_SPI(uint8_t num, const ArchAVR_SPIConfig& config);
    virtual ~ArchAVR_SPI();

    virtual bool init(Device& device) override;
    virtual void reset(int flags) override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual uint8_t ioreg_peek_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    class _PinDriver;
    class _Controller;

    const ArchAVR_SPIConfig& m_config;

    _Controller* m_ctrl;

    InterruptFlag m_intflag;
    bool m_intflag_accessed;

    void update_framerate();
    void update_serial_config();
    void frame_completed();
    void host_selected();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_SPI_H__
