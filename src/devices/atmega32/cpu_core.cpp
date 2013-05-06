#include <inttypes.h>
#include <cstdlib>
#include <cstring>

#include "utils/bit_macros.h"
#include "utils/fail.h"
#include "atmega32.h"
#include "cpu_core.h"
#include "defs.h"

typedef void (*inst_fn_t)(Atmega32Core*, uint16_t);

static inst_fn_t fn_table[65536];
static bool fn_table_initialized = false;

static inline bool is_double_width_instruction(uint16_t opcode)
{
    return
        ((opcode & 0xfc0f) == 0x9000) || // LDS/STS
        ((opcode & 0xfe0c) == 0x940c); // JMP/CALL
}

static uint8_t read_reg(Atmega32Core *core, uint8_t reg)
{
    return core->ram[reg];
}

static uint16_t read_16bit_reg(Atmega32Core *core, uint8_t reg)
{
    return (core->ram[reg+1] << 8) + core->ram[reg];
}

static void write_reg(Atmega32Core *core, uint8_t reg, uint8_t value)
{
    core->ram[reg] = value;
}

static void write_16bit_reg(Atmega32Core *core, uint8_t reg, uint16_t value)
{
    core->ram[reg] = low_byte(value);
    core->ram[reg+1] = high_byte(value);
}

static bool read_reg_bit(Atmega32Core *core, uint8_t reg, uint8_t bit)
{
    return bit_is_set(core->ram[reg], bit);
}

static void write_reg_bit(Atmega32Core *core, uint8_t reg, uint8_t bit, bool value)
{
    chg_bit(core->ram[reg], bit, value);
}

void set_flag(Atmega32Core *core, uint8_t bit, bool value)
{
    write_reg_bit(core, REG_SREG, bit, value);
}

bool get_flag(Atmega32Core *core, uint8_t bit)
{
    return read_reg_bit(core, REG_SREG, bit);
}

uint8_t read_port(Atmega32Core *core, uint8_t port)
{
    uint8_t value = core->ram[IO_BASE + port];
    
    core->master->_onPortRead(port, -1, value);
    
    return value;
}

bool read_port_bit(Atmega32Core *core, uint8_t port, uint8_t bit)
{
    uint8_t value = core->ram[IO_BASE + port];
    
    core->master->_onPortRead(port, bit, value);
    
    return bit_is_set(value, bit);
}

void write_port(Atmega32Core *core, uint8_t port, uint8_t value)
{
    uint8_t prev_val = core->ram[IO_BASE + port];
    
    uint8_t cleared = core->master->_onPortPreWrite(port, -1, value, prev_val);
    core->ram[IO_BASE + port] = value;
    core->master->_onPortWrite(port, -1, value, prev_val, cleared);
}

void write_port_bit(Atmega32Core *core, uint8_t port, uint8_t bit, bool value)
{
    uint8_t prev_val = core->ram[IO_BASE + port];
    uint8_t new_val = (prev_val & ~(1 << bit)) | (value << bit);

    uint8_t cleared = core->master->_onPortPreWrite(port, bit, new_val, prev_val);
    core->ram[IO_BASE + port] = new_val;
    core->master->_onPortWrite(port, -1, new_val, prev_val, cleared);
}

static uint8_t read_mem(Atmega32Core *core, int addr)
{
    if (addr >= RAM_SIZE)
        fail("Read from invalid address (%04x)", addr);
    
    if (addr < IO_BASE) {
        return read_reg(core, addr);
    } else if (addr < SRAM_BASE) {
        return read_port(core, addr - IO_BASE);
    } else
        return core->ram[addr];
}

static void write_mem(Atmega32Core *core, int addr, uint8_t value)
{
    if (addr >= RAM_SIZE)
        fail("Read from invalid address (%04x)", addr);
        
    if (addr < IO_BASE)  {
        write_reg(core, addr, value);
    } else if (addr < SRAM_BASE) {
        write_port(core, addr - IO_BASE, value);
    } else
        core->ram[addr] = value;
}

static uint16_t fetch_next_opcode(Atmega32Core *core)
{
    if (core->pc >= MEGA32_FLASH_SIZE) {
        fail("Attempted fetch from invalid address %s%04x (last instr fetched at %04x)",
            core->pc, core->last_inst_pc);
    }
    
    uint16_t opcode = core->prog_mem.flash[core->pc++];

    return opcode;
}

static void skip_instruction(Atmega32Core *core)
{
    uint16_t opcode = fetch_next_opcode(core);
    
    if (is_double_width_instruction(opcode))
        fetch_next_opcode(core);
}

static void do_jump(Atmega32Core *core, int address)
{
    if ((address < 0) || (address >= MEGA32_FLASH_SIZE)) {
        fail("Attempted jump to invalid address %s%04x (last instr fetched at %04x)",
            (address < 0) ? "-" : "", abs(address), core->last_inst_pc);
    }
    
    core->pc = address;
}

static void do_rel_jump(Atmega32Core *core, int displacement)
{
    do_jump(core, core->pc + displacement);
}

static void push(Atmega32Core *core, uint8_t value)
{
    uint16_t sp = read_16bit_reg(core, REG16_SP);
    write_mem(core, sp, value);
    write_16bit_reg(core, REG16_SP, sp-1);
}

static uint8_t pop(Atmega32Core *core)
{
    uint16_t sp = read_16bit_reg(core, REG16_SP);
    uint8_t value = read_mem(core, ++sp);
    write_16bit_reg(core, REG16_SP, sp);

    return value;
}

void push_word(Atmega32Core *core, uint16_t value)
{
    push(core, low_byte(value));
    push(core, high_byte(value));
}

static uint16_t pop_word(Atmega32Core *core)
{
    return (pop(core) << 8) + pop(core);
}

static void do_load_store(Atmega32Core *core, uint8_t addr_reg, uint16_t displ,
    uint8_t dest_reg, uint8_t incrementing, bool store)
{
    uint16_t a = read_16bit_reg(core, addr_reg);
    
    if (incrementing == 2)
        write_16bit_reg(core, addr_reg, --a);
    if (store) {
        write_mem(core, a + displ, read_reg(core, dest_reg));
    } else {
        write_reg(core, dest_reg, read_mem(core, a + displ));
    }
    if (incrementing == 1)
        write_16bit_reg(core, addr_reg, a+1);
}

static void load_prog_mem(Atmega32Core *core, uint8_t dest_reg, bool post_increment, bool extended)
{
    uint16_t z = read_16bit_reg(core, REG16_Z);
    
    if (extended)
        fail("Extended LPM not supported");

    core->ram[dest_reg] = core->prog_mem.readByte(z);
    
    if (post_increment)
        write_16bit_reg(core, REG16_Z, z+1);
}

static void do_add(Atmega32Core *core, uint8_t dest_reg, uint8_t value, bool carry)
{
    uint8_t d = read_reg(core, dest_reg);
    uint8_t result = d + value + (carry & get_flag(core, FLAG_C));
    
    set_flag(core, FLAG_H, bit_is_set((d & value) | (value & ~result) | (~result & d), 3));
    set_flag(core, FLAG_C, bit_is_set((d & value) | (value & ~result) | (~result & d), 7));
    set_flag(core, FLAG_V, bit_is_set((d & value & ~result) | (~d & ~value & result), 7));
    set_flag(core, FLAG_N, bit_is_set(result, 7));
    set_flag(core, FLAG_Z, !result);
    set_flag(core, FLAG_S, get_flag(core, FLAG_N) ^ get_flag(core, FLAG_V));
    
    write_reg(core, dest_reg, result);
}

static void do_cp_or_sub(Atmega32Core *core, uint8_t dest_reg, uint8_t value, bool carry, bool store)
{
    uint8_t d = read_reg(core, dest_reg);
    uint8_t result = d - value - (carry & get_flag(core, FLAG_C));
    
    set_flag(core, FLAG_H, bit_is_set((~d & value) | (value & result) | (result & ~d), 3));
    set_flag(core, FLAG_C, bit_is_set((~d & value) | (value & result) | (result & ~d), 7));
    set_flag(core, FLAG_V, bit_is_set((d & ~value & ~result) | (~d & value & result), 7));
    set_flag(core, FLAG_N, bit_is_set(result, 7));
    set_flag(core, FLAG_Z, !result & (!carry | get_flag(core, FLAG_Z)));
    set_flag(core, FLAG_S, get_flag(core, FLAG_N) ^ get_flag(core, FLAG_V));
    
    if (store)
        write_reg(core, dest_reg, result);
}

static void set_logical_op_flags(Atmega32Core *core, uint8_t result)
{
    set_flag(core, FLAG_N, bit_is_set(result, 7));
    set_flag(core, FLAG_Z, !result);
    set_flag(core, FLAG_S, get_flag(core, FLAG_N) ^ get_flag(core, FLAG_V));
    set_flag(core, FLAG_V, 0);
}

static void exec_movw(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned r_h : 4;
        unsigned d_h : 4;
        unsigned : 8;
    } *ins = (inst *)&opcode;
    
    write_16bit_reg(core, ins->d_h * 2, read_16bit_reg(core, ins->r_h * 2));
}

static void exec_multiplications(Atmega32Core *core, uint16_t opcode)
{
    union inst {
        struct {
            unsigned r_l : 4;
            unsigned d : 5;
            unsigned r_h : 1;
            unsigned marker : 1;
            unsigned : 5;
        } as_mul;
        struct {
            unsigned r_l : 4;
            unsigned d_l : 4;
            unsigned marker : 1;
            unsigned : 7;
        } as_muls;
        struct {
            unsigned r_l : 3;
            unsigned opcode0 : 1;
            unsigned d_l : 3;
            unsigned opcode1 : 1;
            unsigned : 8;
        } as_other;
    } *ins = (inst *)&opcode;
    
    uint8_t d;
    uint8_t r;
    bool d_signed = false;
    bool r_signed = false;
    bool fractional = false;
    
    if (ins->as_mul.marker) {
        d = ins->as_mul.d;
        r = (ins->as_mul.r_h << 4) + ins->as_mul.r_l;
    } else if (!ins->as_muls.marker) {
        d = 16 + ins->as_muls.d_l;
        r = 16 + ins->as_muls.r_l;
        d_signed = r_signed = true;
    } else {
        d = 16 + ins->as_other.d_l;
        r = 16 + ins->as_other.r_l;
        d_signed = !ins->as_other.opcode0 | ins->as_other.opcode1;
        r_signed = ins->as_other.opcode1;
        fractional = ins->as_other.opcode0 | ins->as_other.opcode1;
    }
    
    if (fractional)
        fail("FMUL instructions not supported");
    
    int d_val = d_signed ? ((int8_t)read_reg(core, d)) : read_reg(core, d);
    int r_val = d_signed ? ((int8_t)read_reg(core, r)) : read_reg(core, r);
    int result = (d_val * r_val) & 0xffff;
    
    write_reg(core, 0, low_byte(result));
    write_reg(core, 1, high_byte(result));
    set_flag(core, FLAG_C, bit_is_set(result, 15));
    set_flag(core, FLAG_Z, !result);
}

static void exec_reg_reg_op(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned r_l : 4;
        unsigned d : 5;
        unsigned r_h : 1;
        unsigned opcode : 6;
    } *ins = (inst *)&opcode;
    
    int op = ins->opcode;
    int d = ins->d;
    int r = (ins->r_h << 4) + ins->r_l;
    uint8_t d_val = read_reg(core, d);
    uint8_t r_val = read_reg(core, r);
    
    switch (op) {
        case 0x01: // CPC
        case 0x02: // SBC
        case 0x05: // CP
        case 0x06: // SUB
            do_cp_or_sub(core, d, r_val, (op <= 0x02), !(op & 0x01));
            break;
        case 0x03: // ADD
        case 0x07: // ADC
            do_add(core, d, r_val, (op == 0x07));
            break;
        case 0x04: // CPSE
            if (d_val == r_val)
                skip_instruction(core);
            break;
        case 0x08: // AND
        case 0x09: // EOR
        case 0x0a: // OR
            switch (op) {
                case 0x08: d_val &= r_val; break;
                case 0x09: d_val ^= r_val; break;
                case 0x0a: d_val |= r_val; break;
            }
            
            write_reg(core, d, d_val);
            set_logical_op_flags(core, d_val);
            break;
        case 0x0b: // MOV
            write_reg(core, d, r_val);
            break;
        default:
            fail("Unsupported two-reg instruction %04x", opcode);
            break;
    }
}

static void exec_reg_imm_op(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned val_l : 4;
        unsigned d_l : 4;
        unsigned val_h : 4;
        unsigned opcode : 4;    
    } *ins = (inst *)&opcode;
    
    int d = 16 + ins->d_l;
    uint8_t d_val = read_reg(core, d);
    uint8_t val = (ins->val_h << 4) + ins->val_l;
    
    switch (ins->opcode) {
        case 0x03: // CPI
            do_cp_or_sub(core, d, val, false, false);
            break;
        case 0x04: // SBCI
            do_cp_or_sub(core, d, val, true, true);
            break;
        case 0x05: // SUBI
            do_cp_or_sub(core, d, val, false, true);
            break;
        case 0x06: // ORI
        case 0x07: // ANDI
            switch (ins->opcode) {
                case 0x06: d_val |= val; break;
                case 0x07: d_val &= val; break;
            }
            
            set_logical_op_flags(core, d_val);
            write_reg(core, d, d_val);
            break;
        case 0x0e: // LDI
            write_reg(core, d, val);
            break;
        default:
            fail("Unsupported immediate instruction %04x", opcode);
            break;
    }
}

static void exec_ldd(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned q_l : 3;
        unsigned use_y : 1;
        unsigned d : 5;
        unsigned store : 1;
        unsigned q_m : 2;
        unsigned : 1;
        unsigned q_h : 1;
        unsigned : 2;    
    } *ins = (inst *)&opcode;
    
    uint8_t q = (ins->q_h << 5) + (ins->q_m << 3) + ins->q_l; 
    
    do_load_store(core, ins->use_y ? REG16_Y : REG16_Z, q, ins->d, 0, ins->store);
}

static void exec_reg_mem_op(Atmega32Core *core, uint16_t opcode)
{
    uint8_t const IND_REGS[4] = { REG16_Z, 0, REG16_Y, REG16_X };
    
    struct inst {
        unsigned opcode : 4;
        unsigned d : 5;
        unsigned store : 1;
        unsigned : 6;
    } *ins = (inst *)&opcode;
    
    int d = ins->d;
    uint8_t d_val = read_reg(core, d);
    bool store = ins->store;
    uint8_t op = ins->opcode;
    uint16_t addr;
  
    switch (op) {
        case 0x00: // LDS/STS
            addr = fetch_next_opcode(core);
            if (store) {
                write_mem(core, addr, d_val);
            } else {
                write_reg(core, d, read_mem(core, addr));
            }
            break;
        case 0x04: case 0x05: case 0x06: case 0x07: // specials
            if (!store) { // LPM
                load_prog_mem(core, d, bit_is_set(op, 0), bit_is_set(op, 1));
            } else { // atomics
                fail("Unsupported atomic instruction %04x", opcode);
            }
            break;
        case 0x01: case 0x02: case 0x09: case 0x0a: case 0x0c: case 0x0d: case 0x0e: // indirect LD/ST
            do_load_store(core, IND_REGS[op >> 2], 0, d, op & 0x03, store);
            break;
        case 0x0f: // PUSH/POP
            if (store) {
                push(core, d_val);
            } else {
                write_reg(core, d, pop(core));
            }
            break;
        default:
            fail("Unsupported reg-mem instruction %04x", opcode);
            break;
    }
}

static void exec_flag_op(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned : 4;
        unsigned bit : 3;
        unsigned clear : 1;
        unsigned : 8;
    } *ins = (inst *)&opcode;
    
    set_flag(core, ins->bit, !ins->clear);
}

static void exec_long_jump(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned addr_l : 1;
        unsigned call : 1;
        unsigned : 2;
        unsigned addr_h : 5;
        unsigned : 7;
    } *ins = (inst *)&opcode;

    int addr = fetch_next_opcode(core);
    
    if (ins->call)
        push_word(core, core->pc);
    
    do_jump(core, addr + (ins->addr_l << 16) + (ins->addr_h << 17));
}

static void exec_single_reg_op(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned opcode : 4;
        unsigned d : 5;
        unsigned : 7;
    } *ins = (inst *)&opcode;
    
    uint8_t d = ins->d;
    uint8_t d_val = read_reg(core, ins->d);
    uint8_t res = 0;
    uint8_t top_bit = 0;
    
    switch (ins->opcode) {
        case 0x00: // COM
            res = ~d_val;
            set_flag(core, FLAG_C, 1);
            set_flag(core, FLAG_V, 0);
            break;
        case 0x01: // NEG
            res = ~d_val + 1;
            set_flag(core, FLAG_H, bit_is_set(res | d_val, 3));
            set_flag(core, FLAG_C, !!res);
            set_flag(core, FLAG_V, (res == 0x80));
            break;
        case 0x02: // SWAP
            res = (d_val << 4) + ((d_val >> 4) & 15);
            break;
        case 0x03: // INC
            res = d_val+1;
            set_flag(core, FLAG_V, (d_val == 0x7f));
            break;
        case 0x05: // ASR
        case 0x06: // LSR
        case 0x07: // ROR
            switch (ins->opcode) {
                case 0x05: top_bit = bit_is_set(d_val, 7); break;
                case 0x06: top_bit = 0; break;
                case 0x07: top_bit = get_flag(core, FLAG_C); break;
            }
            res = ((d_val >> 1) & 0x7f) | (top_bit << 7);
            set_flag(core, FLAG_C, bit_is_set(d_val, 0));
            set_flag(core, FLAG_V, bit_is_set(d_val, 0) ^ top_bit);
            break;
        case 0x0a: // DEC
            res = d_val-1;
            set_flag(core, FLAG_V, (d_val == 0x80));
            break;
        default:
            fail("Unsupported single-reg instruction %04x", opcode);
            break;
    }
    
    if (ins->opcode != 0x02) {
        set_flag(core, FLAG_Z, !res);
        set_flag(core, FLAG_N, bit_is_set(res, 7));
        set_flag(core, FLAG_S, get_flag(core, FLAG_N) ^ get_flag(core, FLAG_V));
    }
    
    write_reg(core, d, res);
}

static void exec_indirect_jump(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned : 4;
        unsigned extended : 1;
        unsigned : 3;
        unsigned call : 1;
        unsigned : 7;
    } *ins = (inst *)&opcode;
    
    if (ins->extended)
        fail("Extended IJMP/ICALL not supported");
    
    if (ins->call)
        push_word(core, core->pc);
    
    do_jump(core, read_16bit_reg(core, REG16_Z));
}

static void exec_return(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned : 4;
        unsigned from_irq : 1;
        unsigned : 11;
    } *ins = (inst *)&opcode;
    
    uint16_t addr = pop_word(core);
    
    do_jump(core, addr);
    
    if (ins->from_irq)
        set_flag(core, FLAG_I, true);
}

static void exec_mcu_control_op(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned : 4;
        unsigned op : 2;
        unsigned : 10;
    } *ins = (inst *)&opcode;
    
    switch (ins->op) {
        case 0x00: // SLEEP
            // TODO: sleep not supported yet, just do nothing
            break;
        case 0x01: // BREAK
            // TODO: not supported yet, just do nothing
            break;
        case 0x02: // WDR
            // TODO: watchdog not supported yet, just do nothing
            break;
        default:
            fail("Unsupported MCU control instruction %04x", opcode);
            break;
    }
}

static void exec_prog_mem_op(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned : 4;
        unsigned extended : 1;
        unsigned store : 1;
        unsigned : 10;
    } *ins = (inst *)&opcode;
    
    if (ins->store)
        fail("SPM instructions not supported");
    
    load_prog_mem(core, 0, 0, ins->extended);
}

static void exec_word_imm_op(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned val_l : 4;
        unsigned d : 2;
        unsigned val_h : 2;
        unsigned subtract : 1;
        unsigned : 7;
    } *ins = (inst *)&opcode;
    
    int d = 24 + (2 * ins->d);
    
    uint16_t d_val = read_16bit_reg(core, d);
    uint16_t value = (ins->val_h << 4) + ins->val_l;
    uint16_t result;
    
    if (!ins->subtract) { // ADIW
        result = d_val + value;
        
        set_flag(core, FLAG_V, bit_is_set(~d_val & result, 15));
        set_flag(core, FLAG_C, bit_is_set(~result & d_val, 15));
    } else { // SBIW
        result = d_val - value;
        
        set_flag(core, FLAG_V, bit_is_set(d_val & ~result, 15));
        set_flag(core, FLAG_C, bit_is_set(result & ~d_val, 15));
    }
    
    set_flag(core, FLAG_N, bit_is_set(result, 15));
    set_flag(core, FLAG_Z, !result);
    set_flag(core, FLAG_S, get_flag(core, FLAG_N) ^ get_flag(core, FLAG_V));
    
    write_16bit_reg(core, d, result);
}

static void exec_io_bit_op(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned bit : 3;
        unsigned port : 5;
        unsigned is_sbix : 1;
        unsigned value : 1;
        unsigned : 6;
    } *ins = (inst *)&opcode;

    if (ins->is_sbix) { // SBIx
        if (read_port_bit(core, ins->port, ins->bit) == ins->value)
            skip_instruction(core);
    } else { // xBI
        write_port_bit(core, ins->port, ins->bit, ins->value);
    }
}

static void exec_io(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned port_l : 4;
        unsigned r : 5;
        unsigned port_h : 2;
        unsigned is_out : 1;
        unsigned : 4;
    } *ins = (inst *)&opcode;

    int port = (ins->port_h << 4) + ins->port_l;
        
    if (ins->is_out) {
        write_port(core, port, read_reg(core, ins->r));
    } else {
        write_reg(core, ins->r, read_port(core, port));
    }
}

static void exec_relative_jump(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        signed displ : 12;
        unsigned call : 1;
        unsigned : 3;
    } *ins = (inst *)&opcode;
    
    if (ins->call)
        push_word(core, core->pc);

    do_rel_jump(core, (int)ins->displ);
}

static void exec_branch(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned bit : 3;
        signed displ : 7;
        unsigned on_zero : 1;
        unsigned : 5;
    } *ins = (inst *)&opcode;
    
    if (get_flag(core, ins->bit) != ins->on_zero)
        do_rel_jump(core, (int)ins->displ);
}

static void exec_bit_op(Atmega32Core *core, uint16_t opcode)
{
    struct inst {
        unsigned bit : 3;
        unsigned : 1;
        unsigned r : 5;
        unsigned val_store : 1;
        unsigned is_branch : 1;
        unsigned : 5;
    } *ins = (inst *)&opcode;

    if (ins->is_branch) { // SBRx
        if (read_reg_bit(core, ins->r, ins->bit) == ins->val_store)
            skip_instruction(core);
    } else { // BLD/BST
        if (ins->val_store) {
            set_flag(core, FLAG_T, read_reg_bit(core, ins->r, ins->bit));
        } else {
            write_reg_bit(core, ins->r, ins->bit, get_flag(core, FLAG_T));
        }
    }
}

static void exec_nop(Atmega32Core *core, uint16_t opcode)
{
}

static void exec_not_implemented(Atmega32Core *core, uint16_t opcode)
{
    fail("Unsupported instruction %04x at PC=%04x", opcode, core->last_inst_pc);
}

static void init_fn_table()
{
    if (fn_table_initialized)
        return;
    
    for (int op = 0; op < 65536; op++) {
        inst_fn_t fn = exec_not_implemented;
        
        if ((op & 0xfe00) == 0x9600) { // ADIW/SBIW
            fn = exec_word_imm_op;
        } else if ((op >= 0x0400) && (op <= 0x2fff)) { // CPC/SBC/ADD/LSL/CPSE/CP/SUB/ADC/ROL/AND/TST/EOR/CLR/OR/MOV
            fn = exec_reg_reg_op;
        } else if ((op & 0xfc00) == 0x9000) { // LD[S]/ST[S]/[E]LPM/XCH/LAx/PUSH/POP
            fn = exec_reg_mem_op;
        } else if (((op >= 0x3000) && (op <= 0x7fff)) || ((op & 0xf000) == 0xe000)) { // CPI/SBCI/SUBI/ORI/SBR/ANDI/CBR
            fn = exec_reg_imm_op;
        } else if ((op & 0xf000) == 0xb000) { // IN/OUT
            fn = exec_io;
        } else if (((op & 0xfe00) == 0x9400) && (((op & 0x0f) <= 0x07) || ((op & 0x0f) == 0x0a))) { // COM/NEG/SWAP/INC/ASR/LSR/ROR/DEC
            fn = exec_single_reg_op;
        } else if ((op & 0xfeef) == 0x9409) { // [E]IJMP/[E]ICALL
            fn = exec_indirect_jump;
        } else if ((op & 0xffef) == 0x9508) { // RET[I]
            fn = exec_return;
        } else if ((op & 0xe000) == 0xc000) { // RJMP/RCALL
            fn = exec_relative_jump;
        } else if ((op & 0xffcf) == 0x95c8) { // [E]LPM/SPM
            fn = exec_prog_mem_op;
        } else if ((op & 0xfc00) == 0x9800) { // CBI/SBI/SBIC/SBIS
            fn = exec_io_bit_op;
        } else if ((op & 0xf800) == 0xf000) { // BRBS/BRBC
            fn = exec_branch;
        } else if ((op & 0xf800) == 0xf800) { // BLD/BST/SBRC/SBRS
            fn = exec_bit_op;
        } else if ((op & 0xff00) == 0x0100) { // MOVW
            fn = exec_movw;
        } else if ((op & 0xd000) == 0x8000) { // LDD/STD Rd, X/Y+q
            fn = exec_ldd;
        } else if ((op & 0xfe0c) == 0x940c) { // JUMP/CALL
            fn = exec_long_jump;
        } else if ((op & 0xff0f) == 0x9408) { // BSET/BCLR 
            fn = exec_flag_op;
        } else if (((op & 0xfe00) == 0x0200) || ((op & 0xfc00) == 0x9c00)) { // [F]MUL[S][U]
            fn = exec_multiplications;
        } else if ((op & 0xffcf) == 0x9588) { // SLEEP/BREAK/WDR
            fn = exec_mcu_control_op;
        } else if (!op) { // NOP
            fn = exec_nop;
        } else if ((op & 0xff0f) == 0x940b) { // DES
            fn = exec_not_implemented;
        }
        
        fn_table[op] = fn;
    }
    
    fn_table_initialized = true;
}

void atmega32_core_init(Atmega32Core *core, Atmega32 *master)
{
    init_fn_table();

    core->master = master;
}

void atmega32_core_step(Atmega32Core *core)
{
    core->last_inst_pc = core->pc;
    
    uint16_t op = fetch_next_opcode(core);
    
    fn_table[op](core, op);
}
