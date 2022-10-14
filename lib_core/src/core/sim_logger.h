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

#include "sim_cycle_timer.h"
#include <cstdarg>


//=======================================================================================

class DLL_EXPORT AVR_LogWriter {

public:

    virtual ~AVR_LogWriter() {}

    virtual void write(cycle_count_t cycle,
                       int level,
                       uint32_t id,
                       const char* format,
                       std::va_list args);

    static AVR_LogWriter* default_writer();
};


//=======================================================================================

class AVR_Logger;

class DLL_EXPORT AVR_LogHandler {

    friend class AVR_Logger;

public:

    AVR_LogHandler();

    void init(AVR_CycleManager& cycle_manager);

    void set_writer(AVR_LogWriter& w);
    AVR_LogWriter& writer();

private:

    AVR_CycleManager* m_cycle_manager;
    AVR_LogWriter* m_writer;

    void write(int lvl, uint32_t id, const char* fmt, std::va_list args);

};

inline void AVR_LogHandler::set_writer(AVR_LogWriter& writer)
{
    m_writer = &writer;
}

inline AVR_LogWriter& AVR_LogHandler::writer()
{
    return *m_writer;
}


//=======================================================================================

class DLL_EXPORT AVR_Logger {

public:

    enum Level {
        Level_Silent = 0,
        Level_Output,
        Level_Error,
        Level_Warning,
        Level_Debug,
        Level_Trace,
    };

    AVR_Logger(uint32_t id, AVR_LogHandler& hdl);
    AVR_Logger(uint32_t id, AVR_Logger* prt = nullptr);

    void set_level(int lvl);
    int level() const;

    void set_parent(AVR_Logger* p);
    AVR_Logger* parent() const;

    void log(int level, const char* format, ...);

    void err(const char* format, ...);
    void wng(const char* format, ...);
    void dbg(const char* format, ...);

protected:

    uint32_t id() const;

    void filtered_write(int lvl, const char* fmt, std::va_list args);
    void write(int lvl, uint32_t id, const char* fmt, std::va_list args);

private:

    uint32_t m_id;
    int m_level;
    AVR_Logger* m_parent;
    AVR_LogHandler* m_handler;

};

inline void AVR_Logger::set_level(int lvl)
{
    m_level = lvl;
}

inline int AVR_Logger::level() const
{
    return m_level;
}

inline uint32_t AVR_Logger::id() const
{
    return m_id;
}

inline void AVR_Logger::set_parent(AVR_Logger* p)
{
    m_parent = p;
}

inline AVR_Logger* AVR_Logger::parent() const
{
    return m_parent;
}


AVR_Logger& AVR_global_logger();

#endif //__YASIMAVR_LOGGER_H__
