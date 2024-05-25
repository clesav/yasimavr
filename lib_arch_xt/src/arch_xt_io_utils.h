/*
 * arch_xt_io_utils.h
 *
 *  Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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


    A significant part of this file is copied from avr-libc, distributed as
    part of the avr-gcc toolchain.
    The licence for these files is as follow:
 */

//=======================================================================================


#ifndef __YASIMAVR_XT_IO_UTILS_H__
#define __YASIMAVR_XT_IO_UTILS_H__

#include <stddef.h>

#define DEF_BITMASK_F(field) \
    bitmask_t(field ## _gp, field ## _gm)

#define DEF_BITMASK_B(bit) \
    bitmask_t(bit ## _bp, bit ## _bm)

#define DEF_REGBIT_F(addr, field) \
    regbit_t(REG_ADDR(addr), field ## _gp, field ## _gm)

#define DEF_REGBIT_B(addr, bit) \
    regbit_t(REG_ADDR(addr), bit ## _bp, bit ## _bm)

#define EXTRACT_F(reg, field) \
    (((reg) & field ## _gm) >> field ## _gp)

#define EXTRACT_B(reg, bit) \
    (((reg) & bit ## _bm) >> bit ## _bp)

#define READ_IOREG(reg) \
    read_ioreg(REG_ADDR(reg))

#define READ_IOREG_F(reg, field) \
    read_ioreg(regbit_t(REG_ADDR(reg), field ## _gp, field ## _gm))

#define READ_IOREG_B(reg, bit) \
    read_ioreg(regbit_t(REG_ADDR(reg), bit ## _bp, bit ## _bm))

#define WRITE_IOREG(reg, value) \
    write_ioreg(REG_ADDR(reg), (value));

#define WRITE_IOREG_F(reg, field, value) \
    write_ioreg(regbit_t(REG_ADDR(reg), field ## _gp, field ## _gm), (value))

#define WRITE_IOREG_B(reg, bit, value) \
    write_ioreg(REG_ADDR(reg), bit ## _bp, (value))

#define TEST_IOREG(reg, bit) \
    test_ioreg(REG_ADDR(reg), bit ## _bp)

#define SET_IOREG(reg, bit) \
    set_ioreg(REG_ADDR(reg), bit ## _bp)

#define CLEAR_IOREG(reg, bit) \
    clear_ioreg(REG_ADDR(reg), bit ## _bp)


#endif //__YASIMAVR_XT_IO_UTILS_H__
