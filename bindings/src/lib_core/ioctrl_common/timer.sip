/*
 * timer.sip
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

class AVR_PrescaledTimer : public AVR_CycleTimer {
%TypeHeaderCode
#include "ioctrl_common/sim_timer.h"
%End

public:
	
	AVR_PrescaledTimer();
	virtual ~AVR_PrescaledTimer();
	
	void init(AVR_CycleManager&, AVR_DeviceLogger&);
	void reset();
	void set_prescaler(uint32_t, uint32_t);
	uint32_t prescaler_factor() const;
	void set_paused(bool);
	void set_timer_delay(uint32_t);
	uint32_t timer_delay() const;
	void update(cycle_count_t = -1);

	virtual cycle_count_t next(cycle_count_t);

	AVR_Signal& signal();
	
	void register_chained_timer(AVR_PrescaledTimer&);
	void unregister_chained_timer(AVR_PrescaledTimer&);

};
