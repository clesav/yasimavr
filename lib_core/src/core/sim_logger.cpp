/*
 * sim_logger.cpp
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

#include "sim_logger.h"


//=======================================================================================

void AVR_LogWriter::write(cycle_count_t cycle,
                          int level,
                          uint32_t id,
                          const char* format,
                          std::va_list args)
{
    std::FILE* f = stdout;

    const char* slvl;
    switch (level) {
        case AVR_AbstractLogger::Level_Trace:
            slvl = "TRA"; break;
        case AVR_AbstractLogger::Level_Debug:
            slvl = "DBG"; break;
        case AVR_AbstractLogger::Level_Warning:
            slvl = "WNG"; break;
        case AVR_AbstractLogger::Level_Error:
            slvl = "ERR"; break;
        case AVR_AbstractLogger::Level_Output:
            slvl= "OUT"; break;
        default:
            slvl = "---"; break;
    }

    std::string sid = id > 0 ? id_to_str(id) : "";

    fprintf(f, "[%08d] %s %s : ", cycle, slvl, sid.c_str());
    vfprintf(f, format, args);
    fprintf(f, "\n");
    fflush(f);
}

static AVR_LogWriter s_default_writer;
AVR_LogWriter const* AVR_DefaultWriter = &s_default_writer;

//=======================================================================================

AVR_AbstractLogger::AVR_AbstractLogger(uint32_t id)
:m_id(id)
,m_level(Level_Error)
{}

void AVR_AbstractLogger::log(int lvl, const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    write(lvl, m_id, format, args);
    va_end(args);
}

void AVR_AbstractLogger::err(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    write(Level_Error, m_id, format, args);
    va_end(args);
}

void AVR_AbstractLogger::wng(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    write(Level_Warning, m_id, format, args);
    va_end(args);
}

void AVR_AbstractLogger::dbg(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    write(Level_Debug, m_id, format, args);
    va_end(args);
}


//=======================================================================================

AVR_RootLogger::AVR_RootLogger(uint32_t id)
:AVR_AbstractLogger(id)
,m_cycle_manager(nullptr)
,m_writer(&s_default_writer)
{}

void AVR_RootLogger::init(AVR_CycleManager& cycle_manager)
{
    m_cycle_manager = &cycle_manager;
}

void AVR_RootLogger::write(int lvl, uint32_t id, const char* fmt, std::va_list args)
{
    if (lvl <= level()) {
        cycle_count_t c = m_cycle_manager ? m_cycle_manager->cycle() : 0;
        m_writer->write(c, lvl, id, fmt, args);
    }
}


//=======================================================================================

static AVR_RootLogger s_global_logger = AVR_RootLogger(0);

AVR_RootLogger& AVR_global_logger()
{
    return s_global_logger;
}


//=======================================================================================

AVR_Logger::AVR_Logger(uint32_t id)
:AVR_AbstractLogger(id)
,m_parent(nullptr)
{}

void AVR_Logger::write(int lvl, uint32_t id, const char* fmt, std::va_list args)
{
    if (lvl <= level() && m_parent)
        m_parent->write(lvl, id, fmt, args);
}
