/*
 * arch_avr_spi.h
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

#ifndef __YASIMAVR_AVR_SPI_H__
#define __YASIMAVR_AVR_SPI_H__

#include "arch_avr_globals.h"
#include "ioctrl_common/sim_spi.h"
#include "core/sim_interrupt.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \ingroup api_spi
   \brief Configuration structure for ArchAVR_SPI
 */
struct ArchAVR_SPIConfig {

    reg_addr_t reg_data;

    regbit_t rb_enable;
    regbit_t rb_int_enable;
    regbit_t rb_int_flag;
    regbit_t rb_mode;

    pin_id_t pin_select;

    regbit_t rb_clock;
    regbit_t rb_clock2x;

    int_vect_t iv_spi;

};

/**
   \ingroup api_spi
   \brief Implementation of a SPI interface for AVR series
   Features:
    - Host/client mode
    - data order, phase and polarity settings have no effect
    - write collision flag not supported

    \sa sim_spi.h
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_SPI : public Peripheral, public SignalHook {

public:

    ArchAVR_SPI(uint8_t num, const ArchAVR_SPIConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    const ArchAVR_SPIConfig& m_config;

    SPI m_spi;

    Pin* m_pin_select;
    bool m_pin_selected;

    InterruptFlag m_intflag;

    void update_framerate();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_SPI_H__
