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

};

extern AVR_LogWriter const* AVR_DefaultWriter;


//=======================================================================================

class DLL_EXPORT AVR_AbstractLogger {

public:

    enum {
        Level_None = 0,
        Level_Output,
        Level_Error,
        Level_Warning,
        Level_Debug,
        Level_Trace,
    };

    AVR_AbstractLogger(uint32_t id);
    virtual ~AVR_AbstractLogger() {}

    void set_level(int lvl);
    int level() const;

    void log(int level, const char* format, ...);

    void err(const char* format, ...);
    void wng(const char* format, ...);
    void dbg(const char* format, ...);

protected:

    uint32_t id() const;

    virtual void write(int lvl, uint32_t id, const char* fmt, std::va_list args) = 0;

private:

    uint32_t m_id;
    int m_level;

    friend class AVR_Logger;

};

inline void AVR_AbstractLogger::set_level(int lvl)
{
    m_level = lvl;
}

inline int AVR_AbstractLogger::level() const
{
    return m_level;
}

inline uint32_t AVR_AbstractLogger::id() const
{
    return m_id;
}


//=======================================================================================

class DLL_EXPORT AVR_RootLogger : public AVR_AbstractLogger {

public:

    AVR_RootLogger(uint32_t id);
    virtual ~AVR_RootLogger() {}

    void init(AVR_CycleManager& cycle_manager);

    void set_writer(AVR_LogWriter& w);
    AVR_LogWriter& writer();

protected:

    virtual void write(int lvl, uint32_t id, const char* fmt, std::va_list args) override;

private:

    AVR_CycleManager* m_cycle_manager;
    AVR_LogWriter* m_writer;

};

inline void AVR_RootLogger::set_writer(AVR_LogWriter& writer)
{
    m_writer = &writer;
}

inline AVR_LogWriter& AVR_RootLogger::writer()
{
    return *m_writer;
}

AVR_RootLogger& AVR_global_logger();


//=======================================================================================

class DLL_EXPORT AVR_Logger : public AVR_AbstractLogger {

public:

    AVR_Logger(uint32_t id);
    virtual ~AVR_Logger() {}

    void set_parent(AVR_AbstractLogger* parent);
    AVR_AbstractLogger* parent();

protected:

    virtual void write(int lvl, uint32_t id, const char* fmt, std::va_list args) override;

private:

    AVR_AbstractLogger* m_parent;

};

inline void AVR_Logger::set_parent(AVR_AbstractLogger* p)
{
    m_parent = p;
}

inline AVR_AbstractLogger* AVR_Logger::parent()
{
    return m_parent;
}

#endif //__YASIMAVR_LOGGER_H__
