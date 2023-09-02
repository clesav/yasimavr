/*
 * sim_logger.h
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

#ifndef __YASIMAVR_LOGGER_H__
#define __YASIMAVR_LOGGER_H__

#include "sim_types.h"
#include "sim_cycle_timer.h"
#include <cstdarg>

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

class AVR_CORE_PUBLIC_API LogWriter {

public:

    virtual ~LogWriter() = default;

    virtual void write(cycle_count_t cycle,
                       int level,
                       ctl_id_t id,
                       const char* format,
                       std::va_list args);

    static LogWriter* default_writer();
};


//=======================================================================================

class Logger;

class AVR_CORE_PUBLIC_API LogHandler {

    friend class Logger;

public:

    LogHandler();

    void init(CycleManager& cycle_manager);

    void set_writer(LogWriter& w);
    LogWriter& writer();

private:

    CycleManager* m_cycle_manager;
    LogWriter* m_writer;

    void write(int lvl, ctl_id_t id, const char* fmt, std::va_list args);

};

inline void LogHandler::set_writer(LogWriter& writer)
{
    m_writer = &writer;
}

inline LogWriter& LogHandler::writer()
{
    return *m_writer;
}


//=======================================================================================

class AVR_CORE_PUBLIC_API Logger {

public:

    enum Level {
        Level_Silent = 0,
        Level_Output,
        Level_Error,
        Level_Warning,
        Level_Debug,
        Level_Trace,
    };

    Logger(ctl_id_t id, LogHandler& hdl);
    explicit Logger(ctl_id_t id, Logger* prt = nullptr);

    void set_level(int lvl);
    int level() const;

    void set_parent(Logger* p);
    Logger* parent() const;

    void log(int level, const char* format, ...);

    void err(const char* format, ...);
    void wng(const char* format, ...);
    void dbg(const char* format, ...);

protected:

    ctl_id_t id() const;

    void filtered_write(int lvl, const char* fmt, std::va_list args);
    void write(int lvl, ctl_id_t id, const char* fmt, std::va_list args);

private:

    ctl_id_t m_id;
    int m_level;
    Logger* m_parent;
    LogHandler* m_handler;

};

inline void Logger::set_level(int lvl)
{
    m_level = lvl;
}

inline int Logger::level() const
{
    return m_level;
}

inline ctl_id_t Logger::id() const
{
    return m_id;
}

inline void Logger::set_parent(Logger* p)
{
    m_parent = p;
}

inline Logger* Logger::parent() const
{
    return m_parent;
}


Logger& global_logger() AVR_CORE_PUBLIC_API;


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_LOGGER_H__
