/*
 * sim_globals.h
 *
 *  Copyright 2023 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_GLOBALS_H__
#define __YASIMAVR_GLOBALS_H__


#if defined _WIN32 || defined __CYGWIN__
  #ifdef YASIMAVR_CORE_DLL
    #ifdef __GNUC__
      #define AVR_CORE_PUBLIC_API __attribute__ ((dllexport))
    #else
      #define AVR_CORE_PUBLIC_API __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define AVR_CORE_PUBLIC_API __attribute__ ((dllimport))
    #else
      #define AVR_CORE_PUBLIC_API __declspec(dllimport)
    #endif
  #endif
#else
  #if __GNUC__ >= 4
    #define AVR_CORE_PUBLIC_API __attribute__ ((visibility ("default")))
  #else
    #define AVR_CORE_PUBLIC_API
  #endif
#endif


#ifdef YASIMAVR_NAMESPACE
    #define YASIMAVR_BEGIN_NAMESPACE namespace YASIMAVR_NAMESPACE {
    #define YASIMAVR_END_NAMESPACE };
    #define YASIMAVR_USING_NAMESPACE using namespace YASIMAVR_NAMESPACE;
    #define YASIMAVR_QUALIFIED_NAME(name) YASIMAVR_NAMESPACE::name
    namespace YASIMAVR_NAMESPACE {}
#else
    #define YASIMAVR_BEGIN_NAMESPACE
    #define YASIMAVR_END_NAMESPACE
    #define YASIMAVR_USING_NAMESPACE
    #define YASIMAVR_QUALIFIED_NAME(name) name
#endif


#endif //__YASIMAVR_GLOBALS_H__
