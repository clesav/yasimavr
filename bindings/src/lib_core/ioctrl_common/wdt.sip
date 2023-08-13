/*
 * wdt.sip
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

class WatchdogTimer : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_wdt.h"
%End

public:

    WatchdogTimer();

    void reset();
    bool ctlreq(ctlreq_id_t, ctlreq_data_t*);

protected:

    void set_timer(uint32_t, uint32_t, uint32_t);
    virtual void timeout() = 0;

};
