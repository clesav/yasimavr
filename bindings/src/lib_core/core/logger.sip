/*
 * logger.sip
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

class AVR_LogWriter {
%TypeHeaderCode
#include "core/sim_logger.h"
%End

public:

    virtual void write(cycle_count_t, int, uint32_t, const char*, std::va_list);

    static AVR_LogWriter* default_writer();

};


//=======================================================================================

class AVR_LogHandler {
%TypeHeaderCode
#include "core/sim_logger.h"
%End

public:

    AVR_LogHandler();

    void init(AVR_CycleManager&);

    void set_writer(AVR_LogWriter&);
    AVR_LogWriter& writer();

};


//=======================================================================================

class AVR_Logger {
%TypeHeaderCode
#include "core/sim_logger.h"
%End

public:

    enum Level /BaseType=IntEnum/ {
        Level_Silent          /PyName=Silent/,
        Level_Output          /PyName=Output/,
        Level_Error           /PyName=Error/,
        Level_Warning         /PyName=Warning/,
        Level_Debug           /PyName=Debug/,
        Level_Trace           /PyName=Trace/,
    };

    AVR_Logger(uint32_t, AVR_LogHandler&);
    AVR_Logger(uint32_t, AVR_Logger* = NULL);

    void set_level(int);
    int level() const;

    void set_parent(AVR_Logger*);
    AVR_Logger* parent() const;

    void log(int, const char*, ...);

    void err(const char*, ...);
    void wng(const char*, ...);
    void dbg(const char*, ...);

protected:

    uint32_t id() const;

    void filtered_write(int, const char*, std::va_list);
    void write(int, uint32_t, const char*, std::va_list);

};

AVR_Logger& AVR_global_logger();

class std::va_list;
