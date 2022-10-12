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

    virtual ~AVR_LogWriter();

    virtual void write(cycle_count_t, int, uint32_t, const char*, std::va_list);

};


//=======================================================================================

class AVR_AbstractLogger {
%TypeHeaderCode
#include "core/sim_logger.h"
%End

public:

    enum /BaseType=IntEnum/ {
        Level_None,
        Level_Output,
        Level_Error,
        Level_Warning,
        Level_Debug,
        Level_Trace,
    };

    AVR_AbstractLogger(uint32_t);
    virtual ~AVR_AbstractLogger();

    void set_level(int);
    int level() const;

    void log(int, const char*, ...);

    void err(const char*, ...);
    void wng(const char*, ...);
    void dbg(const char*, ...);

protected:

    uint32_t id() const;

    virtual void write(int, uint32_t, const char*, std::va_list) = 0;

};


//=======================================================================================

class AVR_RootLogger : public AVR_AbstractLogger {
%TypeHeaderCode
#include "core/sim_logger.h"
%End

public:

    AVR_RootLogger(uint32_t);
    virtual ~AVR_RootLogger();

    void init(AVR_CycleManager&);

    void set_writer(AVR_LogWriter&);
    AVR_LogWriter& writer();

protected:

    virtual void write(int, uint32_t, const char*, std::va_list);

};


AVR_RootLogger& AVR_global_logger();

//=======================================================================================

class AVR_Logger : public AVR_AbstractLogger {
%TypeHeaderCode
#include "core/sim_logger.h"
%End

public:

    AVR_Logger(uint32_t);
    virtual ~AVR_Logger();

    void set_parent(AVR_AbstractLogger*);
    AVR_AbstractLogger* parent();

protected:

    virtual void write(int, uint32_t, const char*, std::va_list);

};

class std::va_list;
