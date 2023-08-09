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

class LogWriter {
%TypeHeaderCode
#include "core/sim_logger.h"
%End

public:

    virtual void write(cycle_count_t, int, ctl_id_t, const char*, std::va_list&)
        [void (cycle_count_t, int, ctl_id_t, const char*, std::va_list)];

    static LogWriter* default_writer();

};


class LogHandler {
%TypeHeaderCode
#include "core/sim_logger.h"
%End

public:

    LogHandler();

    void init(CycleManager&);

    void set_writer(LogWriter&);
    LogWriter& writer();

};


class Logger {
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

    Logger(ctl_id_t, LogHandler&);
    Logger(ctl_id_t, Logger* = NULL);

    void set_level(int);
    int level() const;

    void set_parent(Logger*);
    Logger* parent() const;

    void log(int, const char*, ...);

    void err(const char*, ...);
    void wng(const char*, ...);
    void dbg(const char*, ...);

protected:

    ctl_id_t id() const;

    void filtered_write(int, const char*, std::va_list&)
        [void (int, const char*, std::va_list)];

    void write(int, ctl_id_t, const char*, std::va_list&)
        [void (int, ctl_id_t, const char*, std::va_list)];

};


Logger& global_logger();


class std::va_list;
