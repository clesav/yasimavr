/*
 * arch_xt_io_utils.h
 *
 *  Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>

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


#ifndef __YASIMAVR_XT_IO_UTILS_H__
#define __YASIMAVR_XT_IO_UTILS_H__

#include <stddef.h>

#define DEF_BITSPEC_F(field) \
    bitspec_t(field ## _gp + bitmask_t(field ## _gm).bitcount() - 1, field ## _gp)

#define DEF_BITSPEC_B(bit) \
    bitspec_t(bit ## _bp)

#define DEF_REGBIT_F(addr, field) \
    regbit_t(REG_ADDR(addr), DEF_BITSPEC_F(field))

#define DEF_REGBIT_B(addr, bit) \
    regbit_t(REG_ADDR(addr), DEF_BITSPEC_B(bit))

#define EXTRACT_F(reg, field) \
    (((reg) & field ## _gm) >> field ## _gp)

#define EXTRACT_B(reg, bit) \
    (((reg) & bit ## _bm) >> bit ## _bp)

#define EXTRACT_GC(reg, field) \
    ((reg) & field ## _gm)

#define READ_IOREG(reg) \
    read_ioreg(REG_ADDR(reg))

#define READ_IOREG_F(reg, field) \
    read_ioreg(REG_ADDR(reg), DEF_BITSPEC_F(field))

#define READ_IOREG_B(reg, bit) \
    read_ioreg(REG_ADDR(reg), DEF_BITSPEC_B(bit))

#define READ_IOREG_F_GC(reg, field) \
    (READ_IOREG(reg) & field ## _gm)

#define READ_IOREG_B_GC(reg, field) \
    (READ_IOREG(reg) & field ## _bm)

#define WRITE_IOREG(reg, value) \
    write_ioreg(REG_ADDR(reg), (value));

#define WRITE_IOREG_F(reg, field, value) \
    write_ioreg(DEF_REGBIT_F(reg, field), (value))

#define WRITE_IOREG_B(reg, bit, value) \
    write_ioreg(REG_ADDR(reg), DEF_BITSPEC_B(bit), (value))

#define WRITE_IOREG_F_GC(reg, field, gc_value) \
    WRITE_IOREG_F(reg, field, (gc_value) >> field ## _gp)

#define WRITE_IOREG_B_GC(reg, field, gc_value) \
    WRITE_IOREG_B(reg, field, (gc_value) >> field ## _bp)

#define TEST_IOREG(reg, bit) \
    test_ioreg(REG_ADDR(reg), DEF_BITSPEC_B(bit))

#define SET_IOREG(reg, bit) \
    set_ioreg(REG_ADDR(reg), DEF_BITSPEC_B(bit))

#define CLEAR_IOREG(reg, bit) \
    clear_ioreg(REG_ADDR(reg), DEF_BITSPEC_B(bit))


#endif //__YASIMAVR_XT_IO_UTILS_H__
