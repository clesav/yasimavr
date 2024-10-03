/*
 * sim_cpu.cpp
 *
 *  Copyright 2021-2024 Clement Savergne <csavergne@yahoo.com>

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

/*
 * Significant parts of this file are copied from sim_core.c, part of simavr and
 * distributed under the GNU General Public License version 3.
 * Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>
 * https://github.com/buserror/simavr
 */

//=======================================================================================

#include "sim_core.h"
#include "sim_debug.h"
#include "sim_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

const char sreg_flag_names[8] = {'c', 'z', 'n', 'v', 's', 'h', 't', 'i'};

const char* sreg_to_str(const uint8_t* sreg, char* sreg_str)
{
    for (uint8_t i = 0; i < 8; i++)
        sreg_str[i] = sreg[i] ? sreg_flag_names[i] : '-';

    sreg_str[8] = '\0';
    return sreg_str;
}

//=======================================================================================

#define CPU_READ_GPREG(d) \
    (m_regs[(d)])

#define CPU_WRITE_GPREG(d, v) \
    (m_regs[(d)] = (v))

#define get_flash16le(addr) \
    (m_flash[addr] | (m_flash[addr + 1] << 8))

#define get_d5(o) \
    const uint8_t d = (o >> 4) & 0x1f;

#define get_vd5(o) \
    get_d5(o) \
    const uint8_t vd = CPU_READ_GPREG(d);

#define get_r5(o) \
    const uint8_t r = ((o >> 5) & 0x10) | (o & 0xf);

#define get_d5_a6(o) \
    get_d5(o) \
    const uint8_t a = ((((o >> 9) & 3) << 4) | ((o) & 0xf));

#define get_vd5_s3(o) \
    get_vd5(o) \
    const uint8_t s = o & 7;

#define get_vd5_s3_mask(o) \
    get_vd5_s3(o) \
    const uint8_t mask = 1 << s;

#define get_vd5_vr5(o) \
    get_r5(o) \
    get_d5(o) \
    const uint8_t vd = CPU_READ_GPREG(d), vr = CPU_READ_GPREG(r);

#define get_d5_vr5(o) \
    get_d5(o) \
    get_r5(o) \
    const uint8_t vr = CPU_READ_GPREG(r);

#define get_h4_k8(o) \
    const uint8_t h = 16 + ((o >> 4) & 0xf); \
    const uint8_t k = ((o & 0x0f00) >> 4) | (o & 0xf);

#define get_vh4_k8(o) \
    get_h4_k8(o) \
    const uint8_t vh = CPU_READ_GPREG(h);

#define get_d5_q6(o) \
    get_d5(o) \
    const uint8_t q = ((o & 0x2000) >> 8) | ((o & 0x0c00) >> 7) | (o & 0x7);

#define get_a5(o) \
    const uint8_t a = ((o >> 3) & 0x1f);

#define get_a5_b3(o) \
    get_a5(o) \
    const uint8_t b = o & 0x7;

#define get_a5_b3mask(o) \
    get_a5(o) \
    const uint8_t mask = 1 << (o & 0x7);

#define get_o12(op) \
    const int16_t o = ((int16_t)((op << 4) & 0xffff)) >> 3;

#define get_vp2_k6(o) \
    const uint8_t p = 24 + ((o >> 3) & 0x6); \
    const uint8_t k = ((o & 0x00c0) >> 2) | (o & 0xf); \
    const uint16_t vp = CPU_READ_GPREG(p) | (CPU_READ_GPREG(p + 1) << 8);

#define get_sreg_bit(o) \
    const uint8_t b = (o >> 4) & 7;

#define get_r16le(r) \
    (CPU_READ_GPREG(r) | (CPU_READ_GPREG(r + 1) << 8))

#define set_r16le(r, v) \
    CPU_WRITE_GPREG(r, (v)); \
    CPU_WRITE_GPREG(r + 1, (v) >> 8);

//SREG flag manipulations

#define set_flags_zns(res) \
    m_sreg[SREG_Z] = (res == 0); \
    m_sreg[SREG_N] = (res >> 7) & 1; \
    m_sreg[SREG_S] = m_sreg[SREG_N] ^ m_sreg[SREG_V];

#define set_flags_zns16(res) \
    m_sreg[SREG_Z] = res == 0; \
    m_sreg[SREG_N] = (res >> 15) & 1; \
    m_sreg[SREG_S] = m_sreg[SREG_N] ^ m_sreg[SREG_V];

#define set_flags_add_zns(res, rd, rr) \
    uint8_t add_carry = (rd & rr) | (rr & ~res) | (~res & rd); \
    m_sreg[SREG_H] = (add_carry >> 3) & 1; \
    m_sreg[SREG_C] = (add_carry >> 7) & 1; \
    m_sreg[SREG_V] = (((rd & rr & ~res) | (~rd & ~rr & res)) >> 7) & 1; \
    set_flags_zns(res);

#define set_flags_sub_zns(res, rd, rr) \
    uint8_t sub_carry = (~rd & rr) | (rr & res) | (res & ~rd); \
    m_sreg[SREG_H] = (sub_carry >> 3) & 1; \
    m_sreg[SREG_C] = (sub_carry >> 7) & 1; \
    m_sreg[SREG_V] = (((rd & ~rr & ~res) | (~rd & rr & res)) >> 7) & 1; \
    set_flags_zns(res);

#define set_flags_Rzns(res) \
    if (res) m_sreg[SREG_Z] = 0; \
    m_sreg[SREG_N] = (res >> 7) & 1; \
    m_sreg[SREG_S] = m_sreg[SREG_N] ^ m_sreg[SREG_V];

#define set_flags_sub_Rzns(res, rd, rr) \
    uint8_t sub_carry = (~rd & rr) | (rr & res) | (res & ~rd); \
    m_sreg[SREG_H] = (sub_carry >> 3) & 1; \
    m_sreg[SREG_C] = (sub_carry >> 7) & 1; \
    m_sreg[SREG_V] = (((rd & ~rr & ~res) | (~rd & rr & res)) >> 7) & 1; \
    set_flags_Rzns(res);

#define set_flags_zcvs(res, vr) \
    m_sreg[SREG_Z] = res == 0; \
    m_sreg[SREG_C] = vr & 1; \
    m_sreg[SREG_V] = m_sreg[SREG_N] ^ m_sreg[SREG_C]; \
    m_sreg[SREG_S] = m_sreg[SREG_N] ^ m_sreg[SREG_V]; \

#define set_flags_zcnvs(res, vr) \
    m_sreg[SREG_Z] = res == 0; \
    m_sreg[SREG_C] = vr & 1; \
    m_sreg[SREG_N] = res >> 7; \
    m_sreg[SREG_V] = m_sreg[SREG_N] ^ m_sreg[SREG_C]; \
    m_sreg[SREG_S] = m_sreg[SREG_N] ^ m_sreg[SREG_V]; \

#define set_flags_znv0s(res) \
    m_sreg[SREG_V] = 0; \
    set_flags_zns(res);

#define INVALID_OPCODE \
    do { \
        char msg[50]; \
        sprintf(msg, "Bad opcode 0x%04x at PC=0x%04lx", opcode, m_pc); \
        m_device->crash(CRASH_INVALID_OPCODE, msg); \
    } while(0);


#ifndef YASIMAVR_NO_TRACE

#define TRACE_JUMP \
    if (m_debug_probe) m_debug_probe->_cpu_notify_jump(new_pc)

#define TRACE_CALL \
    if (m_debug_probe) m_debug_probe->_cpu_notify_call(new_pc)

#define TRACE_RET \
    if (m_debug_probe) m_debug_probe->_cpu_notify_ret()

#define TRACE_OP(f, ...) \
    m_device->logger().log(Logger::Level_Trace, "PC=0x%04X SREG=%s | " f, \
                           m_pc, \
                           sreg_to_str(m_sreg, sreg_str), \
                           ##__VA_ARGS__)

#else

#define TRACE_JUMP
#define TRACE_CALL
#define TRACE_RET
#define TRACE_OP(f, ...)

#endif

#define EIND_VALID \
    (use_extended_addressing() && m_config.eind.valid())

#define RAMPZ_VALID \
    (use_extended_addressing() && m_config.rampz.valid())


static bool _is_instruction_32_bits(uint16_t opcode)
{
    uint16_t o = opcode & 0xfe0f;
    return  o == 0x9200 ||  // STS ! Store Direct to Data Space
            o == 0x9000 ||  // LDS Load Direct from Data Space
            o == 0x940c ||  // JMP Long Jump
            o == 0x940d ||  // JMP Long Jump
            o == 0x940e ||  // CALL Long Call to sub
            o == 0x940f;    // CALL Long Call to sub
}

//Main instruction interpreter, copied from the simavr project with some adaptation
cycle_count_t Core::run_instruction()
{

    if (m_pc > m_config.flashend) {
        m_device->crash(CRASH_PC_OVERFLOW, "Program Counter out of bounds");
        return 0;
    }

    if (!m_flash.programmed(m_pc) || !m_flash.programmed(m_pc + 1)) {
        m_device->logger().err("Program Counter at unprogrammed flash address: 0x%04x", m_pc);
        m_device->crash(CRASH_FLASH_ADDR_OVERFLOW, "Invalid flash address");
        return 0;
    }

#ifndef YASIMAVR_NO_ACC_CTRL
    if (m_section_manager && !m_section_manager->fetch_address(m_pc)) {
        m_device->logger().err("CPU fetching a locked flash address: 0x%04x", m_pc);
        m_device->crash(CRASH_ACCESS_REFUSED, "Instruction fetch refused");
        return 0;
    }
#endif

    uint32_t        opcode = get_flash16le(m_pc);
    flash_addr_t    new_pc = m_pc + 2;  // future "default" pc
    int             cycle = 1;
#ifndef YASIMAVR_NO_TRACE
    char            sreg_str[9];
#endif

    switch (opcode & 0xf000) {
        case 0x0000: {
            switch (opcode) {
                case 0x0000: {  // NOP
                    TRACE_OP("nop");
                }   break;
                default: {
                    switch (opcode & 0xfc00) {
                        case 0x0400: {  // CPC -- Compare with carry -- 0000 01rd dddd rrrr
                            get_vd5_vr5(opcode);
                            uint8_t res = vd - vr - m_sreg[SREG_C];
                            TRACE_OP("cpc r%d[%02x], r%d[%02x] = %02x", d, vd, r, vr, res);
                            set_flags_sub_Rzns(res, vd, vr);
                        }   break;
                        case 0x0c00: {  // ADD -- Add without carry -- 0000 11rd dddd rrrr
                            get_vd5_vr5(opcode);
                            uint8_t res = vd + vr;
                            TRACE_OP("add r%d[%02x], r%d[%02x] = %02x", d, vd, r, vr, res);
                            CPU_WRITE_GPREG(d, res);
                            set_flags_add_zns(res, vd, vr);
                        }   break;
                        case 0x0800: {  // SBC -- Subtract with carry -- 0000 10rd dddd rrrr
                            get_vd5_vr5(opcode);
                            uint8_t res = vd - vr - m_sreg[SREG_C];
                            TRACE_OP("sbc r%d[%02x], r%d[%02x] = %02x", d, vd, r, vr, res);
                            CPU_WRITE_GPREG(d, res);
                            set_flags_sub_Rzns(res, vd, vr);
                        }   break;
                        default:
                            switch (opcode & 0xff00) {
                                case 0x0100: {  // MOVW -- Copy Register Word -- 0000 0001 dddd rrrr
                                    uint8_t d = ((opcode >> 4) & 0xf) << 1;
                                    uint8_t r = ((opcode) & 0xf) << 1;
                                    uint16_t vr = get_r16le(r);
                                    TRACE_OP("movw r%d:r%d, r%d:r%d[%04x]", d, d+1, r, r+1, vr);
                                    set_r16le(d, vr);
                                }   break;
                                case 0x0200: {  // MULS -- Multiply Signed -- 0000 0010 dddd rrrr
                                    uint8_t r = 16 + (opcode & 0xf);
                                    uint8_t d = 16 + ((opcode >> 4) & 0xf);
                                    int8_t vr = (int8_t)CPU_READ_GPREG(r);
                                    int8_t vd = (int8_t)CPU_READ_GPREG(d);
                                    int16_t res = vr * vd;
                                    TRACE_OP("muls r%d[%d], r%d[%02x] = %d", r, vr, d, vd, res);
                                    set_r16le(0, res);
                                    m_sreg[SREG_C] = (res >> 15) & 1;
                                    m_sreg[SREG_Z] = res == 0;
                                    cycle++;
                                }   break;
                                case 0x0300: {  // MUL -- Multiply -- 0000 0011 fddd frrr
                                    uint8_t r = 16 + (opcode & 0x7);
                                    uint8_t d = 16 + ((opcode >> 4) & 0x7);
                                    uint8_t vr = CPU_READ_GPREG(r);
                                    uint8_t vd = CPU_READ_GPREG(d);
                                    int16_t res = 0;
                                    uint8_t c = 0;
                                    switch (opcode & 0x0088) {
                                        case 0x00:  // MULSU -- Multiply Signed Unsigned -- 0000 0011 0ddd 0rrr
                                            res = vr * ((int8_t)vd);
                                            c = (res >> 15) & 1;
                                            TRACE_OP("mulsu r%d[%d], r%d[%02x] = %d", r, vr, d, (int8_t)vd, res);
                                            break;
                                        case 0x08:  // FMUL -- Fractional Multiply Unsigned -- 0000 0011 0ddd 1rrr
                                            res = vr * vd;
                                            c = (res >> 15) & 1;
                                            res <<= 1;
                                            TRACE_OP("fmul r%d[%d], r%d[%02x] = %d", r, vr, d, vd, res);
                                            break;
                                        case 0x80:  // FMULS -- Multiply Signed -- 0000 0011 1ddd 0rrr
                                            res = ((int8_t)vr) * ((int8_t)vd);
                                            c = (res >> 15) & 1;
                                            res <<= 1;
                                            TRACE_OP("fmuls r%d[%d], r%d[%02x] = %d", r, (int8_t)vr, d, (int8_t)vd, res);
                                            break;
                                        case 0x88:  // FMULSU -- Multiply Signed Unsigned -- 0000 0011 1ddd 1rrr
                                            res = vr * ((int8_t)vd);
                                            c = (res >> 15) & 1;
                                            res <<= 1;
                                            TRACE_OP("fmulsu r%d[%d], r%d[%02x] = %d", r, vr, d, (int8_t)vd, res);
                                            break;
                                    }
                                    cycle++;
                                    set_r16le(0, res);
                                    m_sreg[SREG_C] = c;
                                    m_sreg[SREG_Z] = res == 0;
                                }   break;
                                default: INVALID_OPCODE;
                            }
                    }
                }
            }
        }   break;

        case 0x1000: {
            switch (opcode & 0xfc00) {
                case 0x1800: {  // SUB -- Subtract without carry -- 0001 10rd dddd rrrr
                    get_vd5_vr5(opcode);
                    uint8_t res = vd - vr;
                    TRACE_OP("sub r%d[%02x], r%d[%02x] = %02x", d, vd, r, vr, res);
                    CPU_WRITE_GPREG(d, res);
                    set_flags_sub_zns(res, vd, vr);
                }   break;
                case 0x1000: {  // CPSE -- Compare, skip if equal -- 0001 00rd dddd rrrr
                    get_vd5_vr5(opcode);
                    uint16_t res = vd == vr;
                    TRACE_OP("cpse r%d[%02x], r%d[%02x] ; Will %s", d, vd, r, vr, res ? "skip" : "continue");
                    if (res) {
                        if (_is_instruction_32_bits(get_flash16le(new_pc))) {
                            new_pc += 4; cycle += 2;
                        } else {
                            new_pc += 2; cycle++;
                        }
                    }
                }   break;
                case 0x1400: {  // CP -- Compare -- 0001 01rd dddd rrrr
                    get_vd5_vr5(opcode);
                    uint8_t res = vd - vr;
                    TRACE_OP("cp r%d[%02x], r%d[%02x] = %02x", d, vd, r, vr, res);
                    set_flags_sub_zns(res, vd, vr);
                }   break;
                case 0x1c00: {  // ADD -- Add with carry -- 0001 11rd dddd rrrr
                    get_vd5_vr5(opcode);
                    uint8_t res = vd + vr + m_sreg[SREG_C];
                    if (r == d) {
                        TRACE_OP("rol r%d[%02x] = %02x", d, vd, res);
                    } else {
                        TRACE_OP("addc r%d[%02x], r%d[%02x] = %02x", d, vd, r, vr, res);
                    }
                    CPU_WRITE_GPREG(d, res);
                    set_flags_add_zns(res, vd, vr);
                }   break;
                default: INVALID_OPCODE;
            }
        }   break;

        case 0x2000: {
            switch (opcode & 0xfc00) {
                case 0x2000: {  // AND -- Logical AND -- 0010 00rd dddd rrrr
                    get_vd5_vr5(opcode);
                    uint8_t res = vd & vr;
                    if (r == d) {
                        TRACE_OP("tst r%d[%02x]", d, vd);
                    } else {
                        TRACE_OP("and r%d[%02x], r%d[%02x] = %02x", d, vd, r, vr, res);
                    }
                    CPU_WRITE_GPREG(d, res);
                    set_flags_znv0s(res);
                }   break;
                case 0x2400: {  // EOR -- Logical Exclusive OR -- 0010 01rd dddd rrrr
                    get_vd5_vr5(opcode);
                    uint8_t res = vd ^ vr;
                    if (r==d) {
                        TRACE_OP("clr r%d[%02x]", d, vd);
                    } else {
                        TRACE_OP("eor r%d[%02x], r%d[%02x] = %02x", d, vd, r, vr, res);
                    }
                    CPU_WRITE_GPREG(d, res);
                    set_flags_znv0s(res);
                }   break;
                case 0x2800: {  // OR -- Logical OR -- 0010 10rd dddd rrrr
                    get_vd5_vr5(opcode);
                    uint8_t res = vd | vr;
                    TRACE_OP("or r%d[%02x], r%d[%02x] = %02x", d, vd, r, vr, res);
                    CPU_WRITE_GPREG(d, res);
                    set_flags_znv0s(res);
                }   break;
                case 0x2c00: {  // MOV -- 0010 11rd dddd rrrr
                    get_d5_vr5(opcode);
                    uint8_t res = vr;
                    TRACE_OP("mov r%d, r%d[%02x] = %02x", d, r, vr, res);
                    CPU_WRITE_GPREG(d, res);
                }   break;
                default: INVALID_OPCODE;
            }
        }   break;

        case 0x3000: {  // CPI -- Compare Immediate -- 0011 kkkk hhhh kkkk
            get_vh4_k8(opcode);
            uint8_t res = vh - k;
            TRACE_OP("cpi r%d[%02x], %02x", h, vh, k);
            set_flags_sub_zns(res, vh, k);
        }   break;

        case 0x4000: {  // SBCI -- Subtract Immediate With Carry -- 0100 kkkk hhhh kkkk
            get_vh4_k8(opcode);
            uint8_t res = vh - k - m_sreg[SREG_C];
            TRACE_OP("sbci r%d[%02x], %02x = %02x", h, vh, k, res);
            CPU_WRITE_GPREG(h, res);
            set_flags_sub_Rzns(res, vh, k);
        }   break;

        case 0x5000: {  // SUBI -- Subtract Immediate -- 0101 kkkk hhhh kkkk
            get_vh4_k8(opcode);
            uint8_t res = vh - k;
            TRACE_OP("subi r%d[%02x], %02x = %02x", h, vh, k, res);
            CPU_WRITE_GPREG(h, res);
            set_flags_sub_zns(res, vh, k);
        }   break;

        case 0x6000: {  // ORI aka SBR -- Logical OR with Immediate -- 0110 kkkk hhhh kkkk
            get_vh4_k8(opcode);
            uint8_t res = vh | k;
            TRACE_OP("ori r%d[%02x], %02x", h, vh, k);
            CPU_WRITE_GPREG(h, res);
            set_flags_znv0s(res);
        }   break;

        case 0x7000: {  // ANDI -- Logical AND with Immediate -- 0111 kkkk hhhh kkkk
            get_vh4_k8(opcode);
            uint8_t res = vh & k;
            TRACE_OP("andi r%d[%02x], %02x", h, vh, k);
            CPU_WRITE_GPREG(h, res);
            set_flags_znv0s(res);
        }   break;

        case 0xa000:
        case 0x8000: {
            /*
             * Load (LDD/STD) store instructions
             *
             * 10q0 qqsd dddd yqqq
             * s = 0 = load, 1 = store
             * y = 16 bits register index, 1 = Y, 0 = X
             * q = 6 bit displacement
             */
            switch (opcode & 0xd008) {
                case 0xa000:
                case 0x8000: {  // LD (LDD) -- Load Indirect using Z -- 10q0 qqsd dddd yqqq
                    uint16_t z = get_r16le(R_Z);
                    get_d5_q6(opcode);
                    if (opcode & 0x0200) {
                        uint8_t vd = CPU_READ_GPREG(d);
                        TRACE_OP("st (Z+%d[%04x]), r%d[%02x]", q, z+q, d, vd);
                        cpu_write_data(z+q, vd);
                    } else {
                        uint8_t vd = cpu_read_data(z+q);
                        TRACE_OP("ld r%d, (Z+%d[%04x])=[%02x]", d, q, z+q, vd);
                        CPU_WRITE_GPREG(d, vd);
                    }
                    cycle += 1; // 2 cycles, 3 for tinyavr
                }   break;
                case 0xa008:
                case 0x8008: {  // LD (LDD) -- Load Indirect using Y -- 10q0 qqsd dddd yqqq
                    uint16_t y = get_r16le(R_Y);
                    get_d5_q6(opcode);
                    if (opcode & 0x0200) {
                        uint8_t vd = CPU_READ_GPREG(d);
                        TRACE_OP("st (Y+%d[%04x]), r%d[%02x]", q, y+q, d, vd);
                        cpu_write_data(y+q, vd);
                    } else {
                        uint8_t vd = cpu_read_data(y+q);
                        TRACE_OP("ld r%d, (Y+%d[%04x])=[%02x]", d, q, y+q, vd);
                        CPU_WRITE_GPREG(d, vd);
                    }
                    cycle += 1; // 2 cycles, 3 for tinyavr
                }   break;
                default: INVALID_OPCODE;
            }
        }   break;

        case 0x9000: {
            /* this is an annoying special case, but at least these lines handle all the SREG set/clear opcodes */
            if ((opcode & 0xff0f) == 0x9408) { // BSET -- 1001 0100 0sss 1000 / BCLR -- 1001 0100 1sss 1000
                get_sreg_bit(opcode);
                m_sreg[b] = opcode & 0x0080 ? 0 : 1;
                TRACE_OP("%s%c", opcode & 0x0080 ? "cl" : "se", sreg_flag_names[b]);
                //On SEI, ensure the following instruction is executed before any interrupt is processed
                if (b == SREG_I && (opcode & 0x0080))
                    start_interrupt_inhibit(1);
            } else switch (opcode) {
                case 0x9588: { // SLEEP -- 1001 0101 1000 1000
                    TRACE_OP("sleep");
                    m_device->ctlreq(AVR_IOCTL_SLEEP, AVR_CTLREQ_SLEEP_CALL);
                }   break;
                case 0x9598: { // BREAK -- 1001 0101 1001 1000
                    TRACE_OP("break");
                    new_pc -= 2;
                    //The break instruction is handled at device level. If it is handled,
                    //we don't progress the PC until the original opcode is restored
                    m_device->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_BREAK);
                }   break;
                case 0x95a8: { // WDR -- Watchdog Reset -- 1001 0101 1010 1000
                    //STATE("wdr\n");
                    m_device->ctlreq(AVR_IOCTL_WTDG, AVR_CTLREQ_WATCHDOG_RESET);
                }   break;
                case 0x95e8:    // SPM -- Store Program Memory -- 1001 0101 1110 1000
                case 0x95f8: {  // SPM -- Store Program Memory -- 1001 0101 1111 1000 (Z post-increment)
                    bool op = opcode & 0x0010;
                    uint32_t z = get_r16le(R_Z);
                    if (RAMPZ_VALID)
                        z |= cpu_read_ioreg(m_config.rampz) << 16;
                    uint16_t w = (CPU_READ_GPREG(1) << 8) | CPU_READ_GPREG(0);
                    NVM_request_t nvm_req = { .kind = 0, .nvm = -1, .addr = z, .data = w, .result = 0, .cycles = 3 };
                    ctlreq_data_t d = { .data = &nvm_req };
                    bool ok = m_device->ctlreq(AVR_IOCTL_NVM, AVR_CTLREQ_NVM_REQUEST, &d);
                    if (ok && nvm_req.result >= 0) {
                        cycle = nvm_req.cycles;
                        TRACE_OP("spm Z[%04x]%s %02x", z, (op ? "+" : ""), w);
                        if (op) {
                            z += 2;
                            if (RAMPZ_VALID)
                                cpu_write_ioreg(m_config.rampz, z >> 16);
                            set_r16le(R_ZL, z);
                        }
                    } else {
                        if (!ok && !m_device->test_option(Device::Option_IgnoreBadCpuLPM))
                            m_device->crash(CRASH_BAD_CPU_IO, "SPM with no NVM controller");

                        new_pc = m_pc;
                        cycle = 0;
                        TRACE_OP("spm Z[%04x]%s %02x - error", z, (op ? "+" : ""), w);
                    }
                }   break;
                case 0x9409:   // IJMP -- Indirect jump -- 1001 0100 0000 1001
                case 0x9419: { // EIJMP -- Indirect jump -- 1001 0100 0001 1001   bit 4 is "extended"
                    int e = opcode & 0x10;
                    if (e && !EIND_VALID)
                        INVALID_OPCODE;
                    uint32_t z = get_r16le(R_Z);
                    if (e)
                        z |= cpu_read_ioreg(m_config.eind) << 16;
                    TRACE_OP("%sijump Z[%04x]", (e ? "e" : ""), z << 1);
                    new_pc = z << 1;
                    cycle++;
                    TRACE_JUMP;
                }   break;
                case 0x9509:   // ICALL -- Indirect Call to Subroutine -- 1001 0101 0000 1001
                case 0x9519: { // EICALL -- Indirect Call to Subroutine -- 1001 0101 0001 1001   bit 8 is "push pc"
                    int e = opcode & 0x10;
                    if (e && !EIND_VALID)
                        INVALID_OPCODE;
                    uint32_t z = get_r16le(R_Z);
                    if (e)
                        z |= cpu_read_ioreg(m_config.eind) << 16;
                    cpu_push_flash_addr(new_pc >> 1);
                    new_pc = z << 1;
                    TRACE_OP("%sicall Z[%04x] SP[%04x]", (e ? "e" : ""), z << 1, read_sp());
                    cycle += use_extended_addressing() ? 3 : 2;
                    TRACE_CALL;
                }   break;
                case 0x9518:    // RETI -- Return from Interrupt -- 1001 0101 0001 1000
                    exec_reti();
                case 0x9508: {  // RET -- Return -- 1001 0101 0000 1000
                    new_pc = cpu_pop_flash_addr() << 1;
                    if (!new_pc) //crash
                        return 0;
                    TRACE_OP("ret%s to 0x%04x SP[%04x]", (opcode & 0x10 ? "i" : ""), new_pc, read_sp());
                    cycle += 1 + (use_extended_addressing() ? 3 : 2);
                    TRACE_RET;
                }   break;
                case 0x95c8:    // LPM -- Load Program Memory R0 <- (Z) -- 1001 0101 1100 1000
                case 0x95d8: {  // ELPM -- Load Program Memory R0 <- (Z) -- 1001 0101 1101 1000
                    int e = opcode & 0x10;
                    if (e && !RAMPZ_VALID)
                        INVALID_OPCODE;
                    uint16_t z = get_r16le(R_Z);
                    if (e)
                        z |= cpu_read_ioreg(m_config.rampz) << 16;
                    int16_t res = cpu_read_flash(z);
                    if (res >= 0) {
                        CPU_WRITE_GPREG(0, (uint8_t) res);
                        cycle += 2; // 3 cycles
                        TRACE_OP("%slpm r0, (Z[%04x]) = %02x", (e ? "e" : ""), z, (uint8_t) res);
                    } else {
                        new_pc = m_pc;
                        cycle = 0;
                        TRACE_OP("%slpm r0, (Z[%04x]) access refused", (e ? "e" : ""), z);
                    }
                } break;
                default: {
                    switch (opcode & 0xfe0f) {
                        case 0x9000: {  // LDS -- Load Direct from Data Space, 32 bits -- 1001 000d dddd 0000
                            get_d5(opcode);
                            uint16_t x = get_flash16le(new_pc);
                            new_pc += 2;
                            uint8_t v = cpu_read_data(x);
                            TRACE_OP("lds r%d[%02x], 0x%04x", d, v, x);
                            CPU_WRITE_GPREG(d, v);
                            cycle++; // 2 cycles
                        }   break;
                        case 0x9005:
                        case 0x9004: {  // LPM -- Load Program Memory -- 1001 000d dddd 01oo
                            get_d5(opcode);
                            uint16_t z = get_r16le(R_Z);
                            int op = opcode & 1;
                            int16_t res = cpu_read_flash(z);
                            if (res >= 0) {
                                TRACE_OP("lpm r0, (Z[%04x]%s) = %02x", z, op ? "+" : "", res);
                                CPU_WRITE_GPREG(d, res);
                                if (op) {
                                    z++;
                                    set_r16le(R_ZL, z);
                                }
                                cycle += 2; // 3 cycles
                            } else {
                                TRACE_OP("lpm r0, (Z[%04x]%s) access refused", z, op ? "+" : "");
                                new_pc = m_pc;
                                cycle = 0;
                            }
                        }   break;
                        case 0x9006:
                        case 0x9007: {  // ELPM -- Extended Load Program Memory -- 1001 000d dddd 01oo
                            if (!RAMPZ_VALID)
                                INVALID_OPCODE;
                            uint32_t z = get_r16le(R_Z) | (cpu_read_ioreg(m_config.rampz) << 16);
                            get_d5(opcode);
                            int op = opcode & 1;
                            int16_t res = cpu_read_flash(z);
                            if (res >= 0) {
                                TRACE_OP("elpm r%d, (Z[%02x:%04x]%s) = %02x", d, z >> 16, z & 0xffff, op ? "+" : "", res);
                                CPU_WRITE_GPREG(d, res);
                                if (op) {
                                    z++;
                                    cpu_write_ioreg(m_config.rampz, z >> 16);
                                    set_r16le(R_ZL, z);
                                }
                                cycle += 2; // 3 cycles
                            } else {
                                TRACE_OP("elpm r%d, (Z[%02x:%04x]%s) access refused", d, z >> 16, z & 0xffff, op ? "+" : "");
                                new_pc = m_pc;
                                cycle = 0;
                            }
                        }   break;
                        /*
                         * Load store instructions
                         *
                         * 1001 00sr rrrr iioo
                         * s = 0 = load, 1 = store
                         * ii = 16 bits register index, 11 = X, 10 = Y, 00 = Z
                         * oo = 1) post increment, 2) pre-decrement
                         */
                        case 0x900c:
                        case 0x900d:
                        case 0x900e: {  // LD -- Load Indirect from Data using X -- 1001 000d dddd 11oo
                            int op = opcode & 3;
                            get_d5(opcode);
                            uint16_t x = get_r16le(R_X);
                            TRACE_OP("ld r%d, %sX[%04x]%s", d, op == 2 ? "--" : "", x, op == 1 ? "++" : "");
                            cycle++; // 2 cycles (1 for tinyavr, except with inc/dec 2)
                            if (op == 2) x--;
                            uint8_t vd = cpu_read_data(x);
                            if (op == 1) x++;
                            set_r16le(R_XL, x);
                            CPU_WRITE_GPREG(d, vd);
                        }   break;
                        case 0x920c:
                        case 0x920d:
                        case 0x920e: {  // ST -- Store Indirect Data Space X -- 1001 001d dddd 11oo
                            int op = opcode & 3;
                            get_vd5(opcode);
                            uint16_t x = get_r16le(R_X);
                            TRACE_OP("st %sX[%04x]%s, r%d[%02x] ", op == 2 ? "--" : "", x, op == 1 ? "++" : "", d, vd);
                            cycle++; // 2 cycles, except tinyavr
                            if (op == 2) x--;
                            cpu_write_data(x, vd);
                            if (op == 1) x++;
                            set_r16le(R_XL, x);
                        }   break;
                        case 0x9009:
                        case 0x900a: {  // LD -- Load Indirect from Data using Y -- 1001 000d dddd 10oo
                            int op = opcode & 3;
                            get_d5(opcode);
                            uint16_t y = get_r16le(R_Y);
                            TRACE_OP("ld r%d, %sY[%04x]%s", d, op == 2 ? "--" : "", y, op == 1 ? "++" : "");
                            cycle++; // 2 cycles, except tinyavr
                            if (op == 2) y--;
                            uint8_t vd = cpu_read_data(y);
                            if (op == 1) y++;
                            set_r16le(R_YL, y);
                            CPU_WRITE_GPREG(d, vd);
                        }   break;
                        case 0x9209:
                        case 0x920a: {  // ST -- Store Indirect Data Space Y -- 1001 001d dddd 10oo
                            int op = opcode & 3;
                            get_vd5(opcode);
                            uint16_t y = get_r16le(R_Y);
                            TRACE_OP("st %sY[%04x]%s, r%d[%02x] ", op == 2 ? "--" : "", y, op == 1 ? "++" : "", d, vd);
                            cycle++;
                            if (op == 2) y--;
                            cpu_write_data(y, vd);
                            if (op == 1) y++;
                            set_r16le(R_YL, y);
                        }   break;
                        case 0x9200: {  // STS -- Store Direct to Data Space, 32 bits -- 1001 001d dddd 0000
                            get_vd5(opcode);
                            uint16_t x = get_flash16le(new_pc);
                            new_pc += 2;
                            TRACE_OP("sts 0x%04x, r%d[%02x]", x, d, vd);
                            cycle++;
                            cpu_write_data(x, vd);
                        }   break;
                        case 0x9001:
                        case 0x9002: {  // LD -- Load Indirect from Data using Z -- 1001 000d dddd 00oo
                            int op = opcode & 3;
                            get_d5(opcode);
                            uint16_t z = get_r16le(R_Z);
                            TRACE_OP("ld r%d, %sZ[%04x]%s", d, op == 2 ? "--" : "", z, op == 1 ? "++" : "");
                            cycle++; // 2 cycles, except tinyavr
                            if (op == 2) z--;
                            uint8_t vd = cpu_read_data(z);
                            if (op == 1) z++;
                            set_r16le(R_ZL, z);
                            CPU_WRITE_GPREG(d, vd);
                        }   break;
                        case 0x9201:
                        case 0x9202: {  // ST -- Store Indirect Data Space Z -- 1001 001d dddd 00oo
                            int op = opcode & 3;
                            get_vd5(opcode);
                            uint16_t z = get_r16le(R_Z);
                            TRACE_OP("st %sZ[%04x]%s, r%d[%02x] ", op == 2 ? "--" : "", z, op == 1 ? "++" : "", d, vd);
                            cycle++; // 2 cycles, except tinyavr
                            if (op == 2) z--;
                            cpu_write_data(z, vd);
                            if (op == 1) z++;
                            set_r16le(R_ZL, z);
                        }   break;
                        case 0x900f: {  // POP -- 1001 000d dddd 1111
                            get_d5(opcode);
                            uint16_t sp = read_sp();
                            if (sp == m_config.ramend) {
                                m_device->crash(CRASH_SP_OVERFLOW, "SP overflow on POP");
                                return 0;
                            }
                            sp++;
                            uint8_t res = cpu_read_data(sp);
                            write_sp(sp);
                            CPU_WRITE_GPREG(d, res);
                            TRACE_OP("pop r%d SP[%04x] = 0x%02x", d, sp, res);
                            cycle++;
                        }   break;
                        case 0x920f: {  // PUSH -- 1001 001d dddd 1111
                            get_vd5(opcode);
                            uint16_t sp = read_sp();
                            if (sp == 0) {
                                m_device->crash(CRASH_SP_OVERFLOW, "SP overflow on PUSH");
                                return 0;
                            }
                            cpu_write_data(sp, vd);
                            write_sp(sp - 1);
                            TRACE_OP("push r%d[%02x] SP[%04x]", d, vd, sp - 1);
                            cycle++;
                        }   break;
                        case 0x9400: {  // COM -- One's Complement -- 1001 010d dddd 0000
                            get_vd5(opcode);
                            uint8_t res = 0xff - vd;
                            TRACE_OP("com r%d[%02x] = %02x", d, vd, res);
                            CPU_WRITE_GPREG(d, res);
                            set_flags_znv0s(res);
                            m_sreg[SREG_C] = 1;
                        }   break;
                        case 0x9401: {  // NEG -- Two's Complement -- 1001 010d dddd 0001
                            get_vd5(opcode);
                            uint8_t res = 0x00 - vd;
                            TRACE_OP("neg r%d[%02x] = %02x", d, vd, res);
                            CPU_WRITE_GPREG(d, res);
                            m_sreg[SREG_H] = ((res >> 3) | (vd >> 3)) & 1;
                            m_sreg[SREG_V] = res == 0x80;
                            m_sreg[SREG_C] = res != 0;
                            set_flags_zns(res);
                        }   break;
                        case 0x9402: {  // SWAP -- Swap Nibbles -- 1001 010d dddd 0010
                            get_vd5(opcode);
                            uint8_t res = (vd >> 4) | (vd << 4) ;
                            TRACE_OP("swap r%d[%02x] = %02x", d, vd, res);
                            CPU_WRITE_GPREG(d, res);
                        }   break;
                        case 0x9403: {  // INC -- Increment -- 1001 010d dddd 0011
                            get_vd5(opcode);
                            uint8_t res = vd + 1;
                            TRACE_OP("inc r%d[%02x] = %02x", d, vd, res);
                            CPU_WRITE_GPREG(d, res);
                            m_sreg[SREG_V] = res == 0x80;
                            set_flags_zns(res);
                        }   break;
                        case 0x9405: {  // ASR -- Arithmetic Shift Right -- 1001 010d dddd 0101
                            get_vd5(opcode);
                            uint8_t res = (vd >> 1) | (vd & 0x80);
                            TRACE_OP("asr r%d[%02x]", d, vd);
                            CPU_WRITE_GPREG(d, res);
                            set_flags_zcnvs(res, vd);
                        }   break;
                        case 0x9406: {  // LSR -- Logical Shift Right -- 1001 010d dddd 0110
                            get_vd5(opcode);
                            uint8_t res = vd >> 1;
                            TRACE_OP("lsr r%d[%02x]", d, vd);
                            CPU_WRITE_GPREG(d, res);
                            m_sreg[SREG_N] = 0;
                            set_flags_zcvs(res, vd);
                        }   break;
                        case 0x9407: {  // ROR -- Rotate Right -- 1001 010d dddd 0111
                            get_vd5(opcode);
                            uint8_t res = (m_sreg[SREG_C] ? 0x80 : 0) | vd >> 1;
                            TRACE_OP("ror r%d[%02x]", d, vd);
                            CPU_WRITE_GPREG(d, res);
                            set_flags_zcnvs(res, vd);
                        }   break;
                        case 0x940a: {  // DEC -- Decrement -- 1001 010d dddd 1010
                            get_vd5(opcode);
                            uint8_t res = vd - 1;
                            TRACE_OP("dec r%d[%02x] = %02x", d, vd, res);
                            CPU_WRITE_GPREG(d, res);
                            m_sreg[SREG_V] = res == 0x7f;
                            set_flags_zns(res);
                        }   break;
                        case 0x940c:
                        case 0x940d: {  // JMP -- Long Call to sub, 32 bits -- 1001 010a aaaa 110a
                            flash_addr_t a = ((opcode & 0x01f0) >> 3) | (opcode & 1);
                            a = (a << 16) | get_flash16le(new_pc);
                            new_pc = a << 1;
                            TRACE_OP("jmp 0x%04x", new_pc);
                            cycle += 2;
                            TRACE_JUMP;
                        }   break;
                        case 0x940e:
                        case 0x940f: {  // CALL -- Long Call to sub, 32 bits -- 1001 010a aaaa 111a
                            flash_addr_t a = ((opcode & 0x01f0) >> 3) | (opcode & 1);
                            a = (a << 16) | get_flash16le(new_pc);
                            cpu_push_flash_addr((new_pc >> 1) + 1);
                            new_pc = a << 1;
                            TRACE_OP("call 0x%04x SP[%04x]", new_pc, read_sp());
                            cycle += 3 + (use_extended_addressing() ? 1 : 0);
                            TRACE_CALL;
                        }   break;

                        default: {
                            switch (opcode & 0xff00) {
                                case 0x9600: {  // ADIW -- Add Immediate to Word -- 1001 0110 KKpp KKKK
                                    get_vp2_k6(opcode);
                                    uint16_t res = vp + k;
                                    TRACE_OP("adiw r%d:r%d[%04x], 0x%02x", p, p + 1, vp, k);
                                    set_r16le(p, res);
                                    m_sreg[SREG_V] = ((~vp & res) >> 15) & 1;
                                    m_sreg[SREG_C] = ((~res & vp) >> 15) & 1;
                                    set_flags_zns16(res);
                                    cycle++;
                                }   break;
                                case 0x9700: {  // SBIW -- Subtract Immediate from Word -- 1001 0111 KKpp KKKK
                                    get_vp2_k6(opcode);
                                    uint16_t res = vp - k;
                                    TRACE_OP("sbiw r%d:r%d[%04x], 0x%02x", p, p + 1, vp, k);
                                    set_r16le(p, res);
                                    m_sreg[SREG_V] = ((vp & ~res) >> 15) & 1;
                                    m_sreg[SREG_C] = ((res & ~vp) >> 15) & 1;
                                    set_flags_zns16(res);
                                    cycle++;
                                }   break;
                                case 0x9800: {  // CBI -- Clear Bit in I/O Register -- 1001 1000 AAAA Abbb
                                    get_a5_b3mask(opcode);
                                    uint8_t va = cpu_read_ioreg(a);
                                    uint8_t res = va & ~mask;
                                    TRACE_OP("cbi r%d[%04x], 0x%02x = 0x%02x", a, va, mask, res);
                                    cpu_write_ioreg(a, res);
                                    cycle++;
                                }   break;
                                case 0x9900: {  // SBIC -- Skip if Bit in I/O Register is Cleared -- 1001 1001 AAAA Abbb
                                    get_a5_b3mask(opcode);
                                    uint8_t va  = cpu_read_ioreg(a);
                                    uint8_t res = va & mask;
                                    TRACE_OP("sbic r%d[%04x], 0x%02x ; Will %s", a, va, mask, res ? "continue" : "skip");
                                    if (!res) {
                                        if (_is_instruction_32_bits(get_flash16le(new_pc))) {
                                            new_pc += 4; cycle += 2;
                                        } else {
                                            new_pc += 2; cycle++;
                                        }
                                    }
                                }   break;
                                case 0x9a00: {  // SBI -- Set Bit in I/O Register -- 1001 1010 AAAA Abbb
                                    get_a5_b3mask(opcode);
                                    uint8_t va = cpu_read_ioreg(a);
                                    uint8_t res = va | mask;
                                    TRACE_OP("sbi r%d[%04x], 0x%02x = %02x", a, va, mask, res);
                                    cpu_write_ioreg(a, res);
                                    cycle++;
                                }   break;
                                case 0x9b00: {  // SBIS -- Skip if Bit in I/O Register is Set -- 1001 1011 AAAA Abbb
                                    get_a5_b3mask(opcode);
                                    uint8_t va = cpu_read_ioreg(a);
                                    uint8_t res = va & mask;
                                    TRACE_OP("sbis r%d[%04x], 0x%02x ; Will %s", a, va, mask, res ? "skip" : "continue");
                                    if (res) {
                                        if (_is_instruction_32_bits(get_flash16le(new_pc))) {
                                            new_pc += 4; cycle += 2;
                                        } else {
                                            new_pc += 2; cycle++;
                                        }
                                    }
                                }   break;
                                default:
                                    switch (opcode & 0xfc00) {
                                        case 0x9c00: {  // MUL -- Multiply Unsigned -- 1001 11rd dddd rrrr
                                            get_vd5_vr5(opcode);
                                            uint16_t res = vd * vr;
                                            TRACE_OP("mul r%d[%02x], r%d[%02x] = %04x", d, vd, r, vr, res);
                                            cycle++;
                                            set_r16le(0, res);
                                            m_sreg[SREG_Z] = res == 0;
                                            m_sreg[SREG_C] = (res >> 15) & 1;
                                        }   break;
                                        default: INVALID_OPCODE;
                                    }
                            }
                        }   break;
                    }
                }   break;
            }
        }   break;

        case 0xb000: {
            switch (opcode & 0xf800) {
                case 0xb800: {  // OUT A,Rr -- 1011 1AAd dddd AAAA
                    get_d5_a6(opcode);
                    uint8_t vd = CPU_READ_GPREG(d);
                    TRACE_OP("out 0x%04x, r%d[%02x]", a, d, vd);
                    cpu_write_ioreg(a, vd);
                }   break;
                case 0xb000: {  // IN Rd,A -- 1011 0AAd dddd AAAA
                    get_d5_a6(opcode);
                    uint8_t va = cpu_read_ioreg(a);
                    TRACE_OP("in r%d 0x%04x[%02x]", d, a, va);
                    CPU_WRITE_GPREG(d, va);
                }   break;
                default: INVALID_OPCODE;
            }
        }   break;

        case 0xc000: {  // RJMP -- 1100 kkkk kkkk kkkk
            get_o12(opcode);
            TRACE_OP("rjmp .%+d [%04x]", o, new_pc + o);
            if (o == -2)
                m_device->ctlreq(AVR_IOCTL_SLEEP, AVR_CTLREQ_SLEEP_PSEUDO);
            new_pc = (new_pc + o) % (m_config.flashend + 1);
            cycle++;
            TRACE_JUMP;
        }   break;

        case 0xd000: {  // RCALL -- 1101 kkkk kkkk kkkk
            get_o12(opcode);
            cpu_push_flash_addr(new_pc >> 1);
            TRACE_OP("rcall .%+d [%04x] SP[%04x]", o, new_pc + o, read_sp());
            cycle += 3 + (use_extended_addressing() ? 1 : 0);
            new_pc = (new_pc + o) % (m_config.flashend + 1);
            // 'rcall .1' is used as a cheap "push 16 bits of room on the stack"
            if (o != 0)
                TRACE_CALL;
        }   break;

        case 0xe000: {  // LDI Rd, K aka SER (LDI r, 0xff) -- 1110 kkkk dddd kkkk
            get_h4_k8(opcode);
            TRACE_OP("ldi r%d, 0x%02x", h, k);
            CPU_WRITE_GPREG(h, k);
        }   break;

        case 0xf000: {
            switch (opcode & 0xfe00) {
                case 0xf000:
                case 0xf200:
                case 0xf400:
                case 0xf600: {  // BRXC/BRXS -- All the SREG branches -- 1111 0Boo oooo osss
                    int16_t k = (((int16_t)(opcode << 6)) >> 8) & 0xFFFE; // offset in bytes
                    uint8_t flag = opcode & 7;
                    int set = (opcode & 0x0400) == 0;       // this bit means BRXC otherwise BRXS
                    int branch = (m_sreg[flag] && set) || (!m_sreg[flag] && !set);
#ifndef YASIMAVR_NO_TRACE
                    const char *opnames[2][8] = {
                            { "brcc", "brne", "brpl", "brvc", "brlt", "brhc", "brtc", "brid"},
                            { "brcs", "breq", "brmi", "brvs", "brge", "brhs", "brts", "brie"},
                    };
                    const char *opname = opnames[set][opcode & 7];
#endif
                    TRACE_OP("%s .%+d [%04x] ; Will %s", opname, k, new_pc + k, branch ? "branch" : "continue");
                    if (branch) {
                        cycle++; // 2 cycles if taken, 1 otherwise
                        new_pc = new_pc + k;
                    }
                }   break;
                case 0xf800:
                case 0xf900: {  // BLD -- Bit Store from T into a Bit in Register -- 1111 100d dddd 0bbb
                    get_vd5_s3_mask(opcode);
                    uint8_t v = (vd & ~mask) | (m_sreg[SREG_T] ? mask : 0);
                    TRACE_OP("bld r%d[%02x], 0x%02x = %02x", d, vd, mask, v);
                    CPU_WRITE_GPREG(d, v);
                }   break;
                case 0xfa00:
                case 0xfb00:{   // BST -- Bit Store into T from bit in Register -- 1111 101d dddd 0bbb
                    get_vd5_s3(opcode)
                    TRACE_OP("bst r%d[%02x], 0x%02x", d, vd, 1 << s);
                    m_sreg[SREG_T] = (vd >> s) & 1;
                }   break;
                case 0xfc00:
                case 0xfe00: {  // SBRS/SBRC -- Skip if Bit in Register is Set/Clear -- 1111 11sd dddd 0bbb
                    get_vd5_s3_mask(opcode)
                    int set = (opcode & 0x0200) != 0;
                    int branch = ((vd & mask) && set) || (!(vd & mask) && !set);
                    TRACE_OP("%s r%d[%02x], 0x%02x ; Will %s", set ? "sbrs" : "sbrc", d, vd, mask, branch ? "skip" : "continue");
                    if (branch) {
                        if (_is_instruction_32_bits(get_flash16le(new_pc))) {
                            new_pc += 4; cycle += 2;
                        } else {
                            new_pc += 2; cycle++;
                        }
                    }
                }   break;
                default: INVALID_OPCODE;
            }
        }   break;

        default: INVALID_OPCODE;

    }

    m_pc = new_pc;

    return cycle;
}


//=======================================================================================

/**
   Insert a soft breakpoint at the flash address set in bp
   This is done by replacing the normal instruction by a BREAK.
   The normal instruction is backed up in the breakpoint structure.

   \param bp breakpoint to insert
 */
void Core::dbg_insert_breakpoint(breakpoint_t& bp)
{
    //Obtain the size of the targeted opcode
    uint32_t curr_opcode = get_flash16le(bp.addr);
    bp.instr_len = _is_instruction_32_bits(curr_opcode) ? 4 : 2;
    //Backup the program instruction
    m_flash.dbg_read(bp.instr, bp.addr, bp.instr_len);
    //Replace the program instruction by a break
    m_flash.dbg_write(AVR_BREAK_OPCODE & 0xFF, bp.addr);
    m_flash.dbg_write(AVR_BREAK_OPCODE >> 8, bp.addr + 1);
}

/**
   Remove a soft breakpoint at the flash address set in bp
   This is done by restoring the initial instruction backed up
   in the breakpoint object.

   \param bp breakpoint to remove
 */
void Core::dbg_remove_breakpoint(breakpoint_t& bp)
{
    //Restore the original instruction in flash
    m_flash.dbg_write(bp.instr, bp.addr, bp.instr_len);
}
