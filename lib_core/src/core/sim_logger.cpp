/*
 * sim_logger.cpp
 *
 *  Copyright 2022-2026 Clement Savergne <csavergne@yahoo.com>

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

#include "sim_logger.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

class StdOutLogWriter : public LogWriter {
public:

    virtual void write(const char* text) override final
    {
        fputs(text, stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }

};


static StdOutLogWriter s_default_writer;

LogWriter* LogWriter::default_writer()
{
    return &s_default_writer;
}

//=======================================================================================

LogHandler::LogHandler()
:m_cycle_manager(nullptr)
,m_writer(&s_default_writer)
{}

void LogHandler::init(CycleManager& cycle_manager)
{
    m_cycle_manager = &cycle_manager;
}

void LogHandler::write(int level, ctl_id_t id, const char* format, std::va_list args)
{
    const char* MSG_FORMAT = "[%08lld] %s %s : ";

    cycle_count_t cycle = m_cycle_manager ? m_cycle_manager->cycle() : 0;

    const char* slvl;
    switch (level) {
        case Logger::Level_Trace:
            slvl = "TRA"; break;
        case Logger::Level_Debug:
            slvl = "DBG"; break;
        case Logger::Level_Warning:
            slvl = "WNG"; break;
        case Logger::Level_Error:
            slvl = "ERR"; break;
        case Logger::Level_Output:
            slvl= "OUT"; break;
        default:
            slvl = "---"; break;
    }

    std::string sid = id.str();

    size_t hdrsize = 18 + sid.length();
    std::va_list args2;
    va_copy(args2, args);
    size_t msgsize = std::vsnprintf(nullptr, 0, format, args2);
    va_end(args2);
    std::vector<char> buf = std::vector<char>(hdrsize + msgsize + 1, '\0');
    std::sprintf(buf.data(), MSG_FORMAT, cycle, slvl, sid.c_str());
    std::vsnprintf(buf.data() + hdrsize, msgsize + 1, format, args);

    m_writer->write(buf.data());
}


//=======================================================================================

Logger::Logger(ctl_id_t id, LogHandler& hdl)
:m_id(id)
,m_level(Level_Error)
,m_parent(nullptr)
,m_handler(&hdl)
{}

Logger::Logger(ctl_id_t id, Logger* prt)
:m_id(id)
,m_level(Level_Error)
,m_parent(prt)
,m_handler(nullptr)
{}

void Logger::log(int lvl, const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    filtered_write(lvl, format, args);
    va_end(args);
}

void Logger::err(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    filtered_write(Level_Error, format, args);
    va_end(args);
}

void Logger::wng(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    filtered_write(Level_Warning, format, args);
    va_end(args);
}

void Logger::dbg(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    filtered_write(Level_Debug, format, args);
    va_end(args);
}

void Logger::filtered_write(int lvl, const char* fmt, std::va_list args)
{
    if (lvl <= level())
        write(lvl, m_id, fmt, args);
}

void Logger::write(int lvl, ctl_id_t id, const char* fmt, std::va_list args)
{
    if (m_parent)
        m_parent->write(lvl, id, fmt, args);
    else if (m_handler)
        m_handler->write(lvl, id, fmt, args);
}


//=======================================================================================

static LogHandler s_global_handler;
static Logger s_global_logger = Logger(sim_id_t(), s_global_handler);

Logger& YASIMAVR_QUALIFIED_NAME(global_logger)()
{
    return s_global_logger;
}
