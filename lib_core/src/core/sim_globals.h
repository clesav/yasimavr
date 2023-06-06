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


#ifdef YASIMAVR_DLL
    #ifdef _MSC_VER
        #define DLL_EXPORT __declspec(dllexport)
    #else
        #define DLL_EXPORT __attribute__((__visibility__("default")))
    #endif
#else
    #ifdef _MSC_VER
        #define DLL_EXPORT __declspec(dllimport)
    #else
        #define DLL_EXPORT
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
