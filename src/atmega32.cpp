#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>

#include "bit_macros.h"
#include "fail.h"
#include "gelf.h"
#include "atmega32.h"

using namespace std;

static char const * const PORT_NAMES[0x40] = {
    "TWBR", "TWSR", "TWAR", "TWDR", "ADCL", "ADCH", "ADCSRA", "ADMUX",
    "ACSR", "UBRRL", "UCSRB", "UCSRA", "UDR", "SPCR", "SPSR", "SPDR",
    "PIND", "DDRD", "PORTD", "PINC", "DDRC", "PORTC", "PINB", "DDRB",
    "PORTB", "PINA", "DDRA", "PORTA", "EECR", "EEDR", "EEARL", "EEARH",
    "UCRSC/UBRRH", "WDTCR", "ASSR", "OCR2", "TCNT2", "TCCR2", "ICR1L", "ICR1H",
    "OCR1BL", "OCR1BH", "OCR1AL", "OCR1AH", "TCNT1L", "TCNT1H", "TCCR1B", "TCCR1A",
    "SFIOR", "OCDR/OSCCAL", "TCNT0", "TCCR0", "MCUCSR", "MCUCR", "TWCR", "SPMCR",
    "TIFR", "TIMSK", "GIFR", "GICR", "OCR0", "SPL", "SPH", "SREG"
};

// Registers
#define REG_XL            0x1a
#define REG_XH            0x1b
#define REG_YL            0x1c
#define REG_YH            0x1d
#define REG_ZL            0x1e
#define REG_ZH            0x1f
#define REG_SPL           0x5d
#define REG_SPH           0x5e
#define REG_SREG          0x5f

// Ports
#define PORT_TWBR         0x00
#define PORT_TWSR         0x01
#define PORT_TWAR         0x02
#define PORT_TWDR         0x03
#define PORT_SPCR         0x0d
#define PORT_SPSR         0x0e
#define PORT_SPDR         0x0f
#define PORT_PIND         0x10
#define PORT_DDRD         0x11
#define PORT_PORTD        0x12
#define PORT_PINC         0x13
#define PORT_DDRC         0x14
#define PORT_PORTC        0x15
#define PORT_PINB         0x16
#define PORT_DDRB         0x17
#define PORT_PORTB        0x18
#define PORT_PINA         0x19
#define PORT_DDRA         0x1a
#define PORT_PORTA        0x1b
#define PORT_TWCR         0x36

// Bit values
#define B_TWINT         7
#define B_TWSTA         5
#define B_TWSTO         4
#define B_TWEN          2
#define B_SPIF          7
#define B_SPIE          7
#define B_SPE           6
#define B_DORD          5
#define B_MSTR          4

// SREG flags
#define FLAG_I          7
#define FLAG_T          6
#define FLAG_H          5
#define FLAG_S          4
#define FLAG_V          3
#define FLAG_N          2
#define FLAG_Z          1
#define FLAG_C          0

// Other
#define TWI_STATUS_START       0x08
#define TWI_STATUS_RESTART     0x10
#define TWI_STATUS_W_SL_ACK    0x18
#define TWI_STATUS_W_SL_NACK   0x20
#define TWI_STATUS_W_DATA_ACK  0x28
#define TWI_STATUS_W_DATA_NACK 0x30
#define TWI_STATUS_R_SL_ACK    0x40
#define TWI_STATUS_R_SL_NACK   0x48
#define TWI_STATUS_R_DATA_ACK  0x50
#define TWI_STATUS_R_DATA_NACK 0x58
#define TWI_STATUS_IDLE        0xf8

static inline bool is_data_port(uint8_t port)
{
    return ((port >= PORT_PIND) && (port <= PORT_PORTA));
}

static inline bool is_PIN_port(uint8_t port)
{
    return is_data_port(port) && (((port - PORT_PIND) % 3) == 0);
}

static inline bool is_DDR_port(uint8_t port)
{
    return is_data_port(port) && (((port - PORT_PIND) % 3) == 1);
}

static inline bool is_PORT_port(uint8_t port)
{
    return is_data_port(port) && (((port - PORT_PIND) % 3) == 2);
}

static inline int pin_for_port(uint8_t port)
{
    if (is_data_port(port)) {
        return MEGA32_PIN_A + 8 * ((PORT_PORTA - port)/3);
    }

    return -1;
}

static inline bool is_twi_port(uint8_t port)
{
    return ((port >= PORT_TWBR) && (port <= PORT_TWDR)) || (port == PORT_TWCR);
}

static inline bool is_spi_port(uint8_t port)
{
    return ((port >= PORT_SPCR) && (port <= PORT_SPDR));
}

static inline bool is_double_width_instruction(uint16_t opcode)
{
    return
        ((opcode & 0xfc0f) == 0x9000) || // LDS/STS
        ((opcode & 0xfe0c) == 0x940c); // JMP/CALL
}

Atmega32::Atmega32()
{
    this->ports = this->ram + MEGA32_IO_BASE;
    
    memset(this->flash, 0xFF, 2*MEGA32_FLASH_SIZE);
    this->reset();
}

void Atmega32::load_program_from_elf(const char *filename)
{
    int fd, temp;
    Elf *elf;
    GElf_Ehdr ehdr;
    size_t count;
    bool found = false;

    if (elf_version(EV_CURRENT) == EV_NONE)
        fail("Cannot init ELF library: %s", elf_errmsg(-1));
    if ((fd = open(filename, O_RDONLY, 0)) < 0)
        fail("Cannot open file '%s'", filename);
    if (!(elf = elf_begin(fd, ELF_C_READ, NULL)))
        fail("elf_begin() failed: %s", elf_errmsg(-1));
    if (elf_kind(elf) != ELF_K_ELF)
        fail("File '%s' is not an ELF object", filename);
    if (gelf_getehdr(elf, &ehdr) == NULL)
        fail("gelf_getehdr() failed: %s", elf_errmsg(-1));
    if ((temp = gelf_getclass(elf)) == ELFCLASSNONE)
        fail("gelf_getclass() failed: %s", elf_errmsg(-1));
    if (temp != ELFCLASS32)
        fail("Invalid ELF class (expected 32-bit)");

    elf_getphdrnum(elf, &count);
    for (int i = 0; i < (int)count; i++) {
        GElf_Phdr phdr;
        gelf_getphdr(elf, i, &phdr);

        if (phdr.p_type != PT_LOAD) continue;
        if (!phdr.p_filesz) continue; // ignore .bss

        if (phdr.p_paddr & 1)
            fail("Segment #%d starts at odd address (%04x)", i, (int)phdr.p_paddr);
        if (phdr.p_paddr + phdr.p_filesz > 2*MEGA32_FLASH_SIZE)
            fail("Segment #%d too long (addr=%04x, length=%d bytes)",
                i, (int)phdr.p_paddr, (int)phdr.p_filesz);

        lseek(fd, phdr.p_offset, SEEK_SET);
        if (read(fd, this->flash + phdr.p_paddr/2, phdr.p_filesz) != (int)phdr.p_filesz)
            fail("Error reading segment #%d from %s (offset=%04x, length=%d bytes)",
                i, filename, (int)phdr.p_offset, (int)phdr.p_filesz);

        if (phdr.p_flags & PF_X)
            found = true;
    }

    if (!found)
        fail("No executable section found");
    
    elf_end(elf);
    close(fd);
}

void Atmega32::reset()
{
    memset(this->ram, 0, MEGA32_RAM_SIZE);
    this->pc = 0;
    
    this->_twiInit();
    this->_spiInit();
}

sim_time_t Atmega32::nextEventTime()
{
    return 0;
}

void Atmega32::step()
{
    this->last_inst_pc = this->pc;
    
    uint16_t op = this->_fetchNextOpcode();
    
    if (!op) { // NOP
        // do nothing
    } else if ((op & 0xff00) == 0x0100) { // MOVW
        this->_execMovW(op);
    } else if (((op & 0xfe00) == 0x0200) || ((op & 0xfc00) == 0x9c00)) { // [F]MUL[S][U]
        this->_execMultiplications(op);
    } else if ((op >= 0x0400) && (op <= 0x2fff)) { // CPC/SBC/ADD/LSL/CPSE/CP/SUB/ADC/ROL/AND/TST/EOR/CLR/OR/MOV
        this->_execRegRegOp(op);
    } else if (((op >= 0x3000) && (op <= 0x7fff)) || ((op & 0xf000) == 0xe000)) { // CPI/SBCI/SUBI/ORI/SBR/ANDI/CBR
        this->_execRegImmOp(op);
    } else if ((op & 0xd000) == 0x8000) { // LDD/STD Rd, X/Y+q
        this->_execLdd(op);
    } else if ((op & 0xfc00) == 0x9000) { // LD[S]/ST[S]/[E]LPM/XCH/LAx/PUSH/POP
        this->_execRegMemOp(op);
    } else if ((op & 0xff0f) == 0x9408) { // BSET/BCLR 
        this->_execFlagOp(op);
    } else if ((op & 0xff0f) == 0x940b) { // DES
        fail("DES instruction not supported in ATMEGA32");
    } else if ((op & 0xfe0c) == 0x940c) { // JUMP/CALL
        this->_execLongJump(op);
    } else if (((op & 0xfe00) == 0x9400) && (((op & 0x0f) <= 0x07) || ((op & 0x0f) == 0x0a))) { // COM/NEG/SWAP/INC/ASR/LSR/ROR/DEC
        this->_execSingleRegOp(op);
    } else if ((op & 0xfeef) == 0x9409) { // [E]IJMP/[E]ICALL
        this->_execIndirectJump(op);
    } else if ((op & 0xffef) == 0x9508) { // RET[I]
        this->_execReturn(op);
    } else if ((op & 0xffcf) == 0x9588) { // SLEEP/BREAK/WDR
        this->_execMcuControlOp(op);
    } else if ((op & 0xffcf) == 0x95c8) { // [E]LPM/SPM
        this->_execProgMemOp(op);
    } else if ((op & 0xfe00) == 0x9600) { // ADIW/SBIW
        this->_execWordImmOp(op);
    } else if ((op & 0xfc00) == 0x9800) { // CBI/SBI/SBIC/SBIS
        this->_execIoBitOp(op);
    } else if ((op & 0xf000) == 0xb000) { // IN/OUT
        this->_execIo(op);
    } else if ((op & 0xe000) == 0xc000) { // RJMP/RCALL
        this->_execRelativeJump(op);
    } else if ((op & 0xf800) == 0xf000) { // BRBS/BRBC
        this->_execBranch(op);
    } else if ((op & 0xf800) == 0xf800) { // BLD/BST/SBRC/SBRS
        this->_execBitOp(op);
    } else {
        fail("Unsupported instruction %04x at PC=%04x", op, this->last_inst_pc);
    }
}

void Atmega32::_execMovW(uint16_t &opcode)
{
    struct inst {
        unsigned r_h : 4;
        unsigned d_h : 4;
        unsigned : 8;
    } *ins = (inst *)&opcode;
    
    this->_writeReg(ins->d_h * 2 + 0, this->_readReg(ins->r_h * 2 + 0));
    this->_writeReg(ins->d_h * 2 + 1, this->_readReg(ins->r_h * 2 + 1));
}

void Atmega32::_execMultiplications(uint16_t &opcode)
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
    
    int d_val = d_signed ? ((int8_t)this->_readReg(d)) : this->_readReg(d);
    int r_val = d_signed ? ((int8_t)this->_readReg(r)) : this->_readReg(r);
    int result = (d_val * r_val) & 0xffff;
    
    this->_writeReg(0, low_byte(result));
    this->_writeReg(1, high_byte(result));
    this->_setFlag(FLAG_C, bit_is_set(result, 15));
    this->_setFlag(FLAG_Z, !result);
}

void Atmega32::_execRegRegOp(uint16_t &opcode)
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
    uint8_t d_val = this->_readReg(d);
    uint8_t r_val = this->_readReg(r);
    
    switch (op) {
        case 0x01: // CPC
        case 0x02: // SBC
        case 0x05: // CP
        case 0x06: // SUB
            this->_doCpOrSub(d, r_val, (op <= 0x02), !(op & 0x01));
            break;
        case 0x03: // ADD
        case 0x07: // ADC
            this->_doAdd(d, r_val, (op == 0x07));
            break;
        case 0x04: // CPSE
            if (d_val == r_val)
                this->_skipInstruction();
            break;
        case 0x08: // AND
        case 0x09: // EOR
        case 0x0a: // OR
            switch (op) {
                case 0x08: d_val &= r_val; break;
                case 0x09: d_val ^= r_val; break;
                case 0x0a: d_val |= r_val; break;
            }
            
            this->_writeReg(d, d_val);
            this->_setLogicalOpFlags(d_val);
            break;
        case 0x0b: // MOV
            this->_writeReg(d, r_val);
            break;
        default:
            fail("Unsupported two-reg instruction %04x", opcode);
    }
}

void Atmega32::_execRegImmOp(uint16_t &opcode)
{
    struct inst {
        unsigned val_l : 4;
        unsigned d_l : 4;
        unsigned val_h : 4;
        unsigned opcode : 4;    
    } *ins = (inst *)&opcode;
    
    int d = 16 + ins->d_l;
    uint8_t d_val = this->_readReg(d);
    uint8_t val = (ins->val_h << 4) + ins->val_l;
    
    switch (ins->opcode) {
        case 0x03: // CPI
            this->_doCpOrSub(d, val, false, false);
            break;
        case 0x04: // SBCI
            this->_doCpOrSub(d, val, true, true);
            break;
        case 0x05: // SUBI
            this->_doCpOrSub(d, val, false, true);
            break;
        case 0x06: // ORI
        case 0x07: // ANDI
            switch (ins->opcode) {
                case 0x06: d_val |= val; break;
                case 0x07: d_val &= val; break;
            }
            
            this->_setLogicalOpFlags(d_val);
            this->_writeReg(d, d_val);
            break;
        case 0x0e: // LDI
            this->_writeReg(d, val);
            break;
        default:
            fail("Unsupported immediate instruction %04x", opcode);
    }
}

void Atmega32::_execLdd(uint16_t &opcode)
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
    
    this->_doLoadStore(ins->use_y ? REG_YL : REG_ZL, q, ins->d, 0, ins->store);
}

void Atmega32::_execRegMemOp(uint16_t &opcode)
{
    uint8_t const IND_REGS[4] = { REG_ZL, 0, REG_YL, REG_XL };
    
    struct inst {
        unsigned opcode : 4;
        unsigned d : 5;
        unsigned store : 1;
        unsigned : 6;
    } *ins = (inst *)&opcode;
    
    int d = ins->d;
    uint8_t d_val = this->_readReg(d);
    bool store = ins->store;
    uint8_t op = ins->opcode;
    uint16_t addr;
  
    switch (op) {
        case 0x00: // LDS/STS
            addr = this->_fetchNextOpcode();
            if (store) {
                this->_writeMem(addr, d_val);
            } else {
                this->_writeReg(d, this->_readMem(addr));
            }
            break;
        case 0x04: case 0x05: case 0x06: case 0x07: // specials
            if (!store) { // LPM
                this->_loadProgMem(d, bit_is_set(op, 0), bit_is_set(op, 1));
            } else { // atomics
                fail("Unsupported atomic instruction %04x", opcode);
            }
            break;
        case 0x01: case 0x02: case 0x09: case 0x0a: case 0x0c: case 0x0d: case 0x0e: // indirect LD/ST
            this->_doLoadStore(IND_REGS[op >> 2], 0, d, op & 0x03, store);
            break;
        case 0x0f: // PUSH/POP
            if (store) {
                this->_push(d_val);
            } else {
                this->_writeReg(d, this->_pop());
            }
            break;
        default:
            fail("Unsupported reg-mem instruction %04x", opcode);
    }
}

void Atmega32::_execFlagOp(uint16_t &opcode)
{
    struct inst {
        unsigned : 4;
        unsigned bit : 3;
        unsigned clear : 1;
        unsigned : 8;
    } *ins = (inst *)&opcode;
    
    this->_setFlag(ins->bit, !ins->clear);
}

void Atmega32::_execLongJump(uint16_t &opcode)
{
    struct inst {
        unsigned addr_l : 1;
        unsigned call : 1;
        unsigned : 2;
        unsigned addr_h : 5;
        unsigned : 7;
    } *ins = (inst *)&opcode;

    int addr = this->_fetchNextOpcode();
    
    if (ins->call)
        this->_pushWord(this->pc);
    
    this->_doJump(addr + (ins->addr_l << 16) + (ins->addr_h << 17));
}

void Atmega32::_execSingleRegOp(uint16_t &opcode)
{
    struct inst {
        unsigned opcode : 4;
        unsigned d : 5;
        unsigned : 7;
    } *ins = (inst *)&opcode;
    
    uint8_t d = ins->d;
    uint8_t d_val = this->_readReg(ins->d);
    uint8_t res;
    
    switch (ins->opcode) {
        case 0x00: // COM
            res = ~d_val;
            this->_setFlag(FLAG_C, 1);
            this->_setFlag(FLAG_V, 0);
            break;
        case 0x01: // NEG
            res = ~d_val + 1;
            this->_setFlag(FLAG_H, bit_is_set((res | d_val), 3));
            this->_setFlag(FLAG_C, !!res);
            this->_setFlag(FLAG_V, (res == 0x80));
            break;
        case 0x02: // SWAP
            res = (d_val << 4) + ((d_val >> 4) & 15);
            break;
        case 0x03: // INC
            res = d_val+1;
            this->_setFlag(FLAG_V, (d_val == 0x7f));
            break;
        case 0x05: // ASR
        case 0x06: // LSR
        case 0x07: // ROR
            uint8_t top_bit;
            switch (ins->opcode) {
                case 0x05: top_bit = bit_is_set(d_val, 7); break;
                case 0x06: top_bit = 0; break;
                case 0x07: top_bit = this->_getFlag(FLAG_C); break;
            }
            res = ((d_val >> 1) & 0x7f) | (top_bit << 7);
            this->_setFlag(FLAG_C, bit_is_set(d_val, 0));
            this->_setFlag(FLAG_V, bit_is_set(d_val, 0) ^ top_bit);
            break;
        case 0x0a: // DEC
            res = d_val-1;
            this->_setFlag(FLAG_V, (d_val == 0x80));
            break;
        default:
            fail("Unsupported single-reg instruction %04x", opcode);
    }
    
    if (ins->opcode == 0x02) {
        this->_setFlag(FLAG_Z, !res);
        this->_setFlag(FLAG_N, bit_is_set(res, 7));
        this->_setFlag(FLAG_S, this->_getFlag(FLAG_N) ^ this->_getFlag(FLAG_V));
    }
    
    this->_writeReg(d, res);
}

void Atmega32::_execIndirectJump(uint16_t &opcode)
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
        this->_pushWord(this->pc);
    
    this->_doJump(*(uint16_t *)(this->ram + REG_ZL));
}

void Atmega32::_execReturn(uint16_t &opcode)
{
    struct inst {
        unsigned : 4;
        unsigned from_irq : 1;
        unsigned : 11;
    } *ins = (inst *)&opcode;
    
    uint16_t addr = this->_popWord();
    
    this->_doJump(addr);
    
    if (ins->from_irq)
        this->_setFlag(FLAG_I, true);
}

void Atmega32::_execMcuControlOp(uint16_t &opcode)
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
    }
}

void Atmega32::_execProgMemOp(uint16_t &opcode)
{
    struct inst {
        unsigned : 4;
        unsigned extended : 1;
        unsigned store : 1;
        unsigned : 10;
    } *ins = (inst *)&opcode;
    
    if (ins->store)
        fail("SPM instructions not supported");
    
    this->_loadProgMem(0, 0, ins->extended);
}

void Atmega32::_execWordImmOp(uint16_t &opcode)
{
    struct inst {
        unsigned val_l : 4;
        unsigned d : 2;
        unsigned val_h : 2;
        unsigned subtract : 1;
        unsigned : 7;
    } *ins = (inst *)&opcode;
    
    int d = 24 + (2 * ins->d);
    
    uint16_t d_val = (this->_readReg(d+1) << 8) + this->_readReg(d);
    uint16_t value = (ins->val_h << 4) + ins->val_l;
    uint16_t result;
    
    if (!ins->subtract) { // ADIW
        result = d_val + value;
        
        this->_setFlag(FLAG_V, bit_is_set(~d_val & result, 15));
        this->_setFlag(FLAG_C, bit_is_set(~result & d_val, 15));
    } else { // SBIW
        result = d_val - value;
        
        this->_setFlag(FLAG_V, bit_is_set(d_val & ~result, 15));
        this->_setFlag(FLAG_C, bit_is_set(result & ~d_val, 15));
    }
    
    this->_setFlag(FLAG_N, bit_is_set(result, 15));
    this->_setFlag(FLAG_Z, !result);
    this->_setFlag(FLAG_S, this->_getFlag(FLAG_N) ^ this->_getFlag(FLAG_V));
    
    this->_writeReg(d, low_byte(result));
    this->_writeReg(d+1, high_byte(result));
}

void Atmega32::_execIoBitOp(uint16_t &opcode)
{
    struct inst {
        unsigned bit : 3;
        unsigned port : 5;
        unsigned is_sbix : 1;
        unsigned value : 1;
        unsigned : 6;
    } *ins = (inst *)&opcode;

    if (ins->is_sbix) { // SBIx
        if (this->_readPortBit(ins->port, ins->bit) == ins->value)
            this->_skipInstruction();
    } else { // xBI
        this->_writePortBit(ins->port, ins->bit, ins->value);
    }
}

void Atmega32::_execIo(uint16_t &opcode)
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
        this->_writePort(port, this->_readReg(ins->r));
    } else {
        this->_writeReg(ins->r, this->_readPort(port));
    }
}

void Atmega32::_execRelativeJump(uint16_t &opcode)
{
    struct inst {
        signed displ : 12;
        unsigned call : 1;
        unsigned : 3;
    } *ins = (inst *)&opcode;
    
    if (ins->call)
        this->_pushWord(this->pc);

    this->_doRelJump((int)ins->displ);
}

void Atmega32::_execBranch(uint16_t &opcode)
{
    struct inst {
        unsigned bit : 3;
        signed displ : 7;
        unsigned on_zero : 1;
        unsigned : 5;
    } *ins = (inst *)&opcode;
    
    if (this->_getFlag(ins->bit) != ins->on_zero)
        this->_doRelJump((int)ins->displ);
}

void Atmega32::_execBitOp(uint16_t &opcode)
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
        if (this->_readRegBit(ins->r, ins->bit) == ins->val_store)
            this->_skipInstruction();
    } else { // BLD/BST
        if (ins->val_store) {
            this->_setFlag(FLAG_T, this->_readRegBit(ins->r, ins->bit));
        } else {
            this->_writeRegBit(ins->r, ins->bit, this->_getFlag(FLAG_T));
        }
    }
}

void Atmega32::_doAdd(uint8_t dest_reg, uint8_t value, bool carry)
{
    uint8_t d = this->_readReg(dest_reg);
    uint8_t result = d + value + (carry & this->_getFlag(FLAG_C));
    
    this->_setFlag(FLAG_H, bit_is_set((d & value) | (value & ~result) | (~result & d), 3));
    this->_setFlag(FLAG_C, bit_is_set((d & value) | (value & ~result) | (~result & d), 7));
    this->_setFlag(FLAG_V, (d & value & ~result) | (~d & ~value & result));
    this->_setFlag(FLAG_N, bit_is_set(result, 7));
    this->_setFlag(FLAG_Z, !result);
    this->_setFlag(FLAG_S, this->_getFlag(FLAG_N) ^ this->_getFlag(FLAG_V));
    
    this->_writeReg(dest_reg, result);
}

void Atmega32::_doCpOrSub(uint8_t dest_reg, uint8_t value, bool carry, bool store)
{
    uint8_t d = this->_readReg(dest_reg);
    uint8_t result = d - value - (carry & this->_getFlag(FLAG_C));
    
    this->_setFlag(FLAG_H, bit_is_set((~d & value) | (value & result) | (result & ~d), 3));
    this->_setFlag(FLAG_C, bit_is_set((~d & value) | (value & result) | (result & ~d), 7));
    this->_setFlag(FLAG_V, (d & ~value & ~result) | (~d & value & result));
    this->_setFlag(FLAG_N, bit_is_set(result, 7));
    this->_setFlag(FLAG_Z, !result & (!carry | this->_getFlag(FLAG_Z)));
    this->_setFlag(FLAG_S, this->_getFlag(FLAG_N) ^ this->_getFlag(FLAG_V));
    
    if (store)
        this->_writeReg(dest_reg, result);
}

void Atmega32::_setLogicalOpFlags(uint8_t result)
{
    this->_setFlag(FLAG_N, bit_is_set(result, 7));
    this->_setFlag(FLAG_Z, !result);
    this->_setFlag(FLAG_S, this->_getFlag(FLAG_N) ^ this->_getFlag(FLAG_V));
    this->_setFlag(FLAG_V, 0);
}

uint16_t Atmega32::_fetchNextOpcode()
{
    if (this->pc >= MEGA32_FLASH_SIZE) {
        fail("Attempted fetch from invalid address %s%04x (last instr fetched at %04x)",
            this->pc, this->last_inst_pc);
    }
    
    uint16_t opcode = this->flash[this->pc++];

    return opcode;
}

void Atmega32::_skipInstruction()
{
    uint16_t opcode = this->_fetchNextOpcode();
    
    if (is_double_width_instruction(opcode))
        this->_fetchNextOpcode();
}

void Atmega32::_doJump(int address)
{
    if ((address < 0) || (address >= MEGA32_FLASH_SIZE)) {
        fail("Attempted jump to invalid address %s%04x (last instr fetched at %04x)",
            (address < 0) ? "-" : "", abs(address), this->last_inst_pc);
    }
    
    this->pc = address;
}

void Atmega32::_doRelJump(int displacement)
{
    this->_doJump(this->pc + displacement);
}

void Atmega32::_setFlag(uint8_t bit, bool set)
{
    if (set) {
        set_bit(this->ram[REG_SREG], bit);
    } else {
        clear_bit(this->ram[REG_SREG], bit);
    }
}

bool Atmega32::_getFlag(uint8_t bit)
{
    return bit_is_set(this->ram[REG_SREG], bit);
}

uint8_t Atmega32::_readReg(uint8_t reg)
{
    return this->ram[reg];
}

void Atmega32::_writeReg(uint8_t reg, uint8_t value)
{
    this->ram[reg] = value;
}

bool Atmega32::_readRegBit(uint8_t reg, uint8_t bit)
{
    return bit_is_set(this->ram[reg], bit);
}

void Atmega32::_writeRegBit(uint8_t reg, uint8_t bit, bool value)
{
    if (value) {
        set_bit(this->ram[reg], bit);
    } else {
        clear_bit(this->ram[reg], bit);
    }
}

uint8_t Atmega32::_readPort(uint8_t port)
{
    uint8_t value = this->ports[port];
    
    this->_onPortRead(port, -1, value);
    
    return value;
}

bool Atmega32::_readPortBit(uint8_t port, uint8_t bit)
{
    uint8_t value = this->ports[port];
    
    this->_onPortRead(port, bit, value);
    
    return bit_is_set(value, bit);
}

void Atmega32::_writePort(uint8_t port, uint8_t value)
{
    uint8_t prev_val = this->ports[port];
    
    this->_onPortWrite(port, -1, value, prev_val);
    
    this->ports[port] = value;
}

void Atmega32::_writePortBit(uint8_t port, uint8_t bit, bool value)
{
    uint8_t prev_val = this->ports[port];
    uint8_t new_val = (prev_val & ~(1 << bit)) | (value << bit);
    
    this->_onPortWrite(port, bit, new_val, prev_val);
    
    this->ports[port] = new_val;
}

uint8_t Atmega32::_readMem(int addr)
{
    if (addr >= MEGA32_RAM_SIZE)
        fail("Read from invalid address (%04x)", addr);
    
    if (addr < MEGA32_IO_BASE) {
        return this->_readReg(addr);
    } else if ((addr >= MEGA32_IO_BASE) && (addr < MEGA32_SRAM_BASE)) {
        return this->_readPort(addr - MEGA32_IO_BASE);
    }
    
    return this->ram[addr];
}

void Atmega32::_writeMem(int addr, uint8_t value)
{
    if (addr >= MEGA32_RAM_SIZE)
        fail("Read from invalid address (%04x)", addr);
        
    if (addr < MEGA32_IO_BASE)  {
        this->_writeReg(addr, value);
    } else if ((addr >= MEGA32_IO_BASE) && (addr < MEGA32_SRAM_BASE)) {
        this->_writePort(addr - MEGA32_IO_BASE, value);
    } else {
        this->ram[addr] = value;
    }
}

void Atmega32::_push(uint8_t value)
{
    uint16_t *sp = (uint16_t *)(this->ram + REG_SPL);
    
    this->_writeMem((*sp)--, value);
}

uint8_t Atmega32::_pop()
{
    uint16_t *sp = (uint16_t *)(this->ram + REG_SPL);
 
    uint8_t value = this->_readMem(++(*sp));

    return value;
}

void Atmega32::_pushWord(uint16_t value)
{
    this->_push(low_byte(value));
    this->_push(high_byte(value));
}

uint16_t Atmega32::_popWord()
{
    return (this->_pop() << 8) + this->_pop();
}

void Atmega32::_doLoadStore(uint8_t addr_reg, uint16_t displ, uint8_t dest_reg, uint8_t incrementing, bool store)
{
    uint16_t *a = (uint16_t *)(this->ram + addr_reg);
    
    if (incrementing == 2) (*a)--;
    if (store) {
        this->_writeMem(*a + displ, this->_readReg(dest_reg));
    } else {
        this->_writeReg(dest_reg, this->_readMem(*a + displ));
    }
    if (incrementing == 1) (*a)++;
}

void Atmega32::_loadProgMem(uint8_t dest_reg, bool post_increment, bool extended)
{
    uint16_t *z = (uint16_t *)(this->ram + REG_ZL);
    
    if (extended)
        fail("Extended LPM not supported");
    
    if (*z >= 2*MEGA32_FLASH_SIZE)
        fail("LPM from invalid program memory address (%04x)", *z);
    
    this->ram[dest_reg] = ((uint8_t *)this->flash)[*z];
    if (post_increment) (*z)++;
}

void Atmega32::_onPortRead(uint8_t port, int8_t bit, uint8_t &value)
{
    if (port >= REG_SPL-MEGA32_IO_BASE)
        return;
    
    if (is_twi_port(port)) {
        this->_twiHandleRead(port, bit, value);
    }
    if (is_spi_port(port)) {
        this->_spiHandleRead(port, bit, value);
    }
    if (is_data_port(port)) {
        this->_handleDataPortRead(port, bit, value);
    }
    /*
    if (bit < 0) {
        printf("%04x: IN %s(%02x) == %02x\n", this->last_inst_pc * 2, PORT_NAMES[port], port,
            value);
    } else {
        printf("%04x: IN %s(%02x).%d == %02x\n", this->last_inst_pc * 2, PORT_NAMES[port], port, bit,
            (value >> bit) & 1);
    }
    */
}

void Atmega32::_onPortWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    if (port >= REG_SPL-MEGA32_IO_BASE)
        return;
        
    if (is_twi_port(port)) {
        this->_twiHandleWrite(port, bit, value, prev_val);
    }
    if (is_spi_port(port)) {
        this->_spiHandleWrite(port, bit, value, prev_val);
    }
    if (is_data_port(port)) {
        this->_handleDataPortWrite(port, bit, value, prev_val);
    }
    /*
    if (bit < 0) {
        printf("%04x: OUT %s(%02x) <- %02x (was %02x)\n", this->last_inst_pc * 2, PORT_NAMES[port], port,
            value, prev_val);
    } else {
        printf("%04x: OUT %s(%02x).%d <- %d (was %d)\n", this->last_inst_pc * 2, PORT_NAMES[port], port, bit,
            (value >> bit) & 1, (prev_val >> bit) & 1);
    }*/
}

void Atmega32::addPinMonitor(int pin, PinMonitor* monitor)
{
    if ((pin < 0) || (pin >= MEGA32_PIN_COUNT))
        fail("No such pin in ATMEGA32 (no. %02x)", pin);
    
    this->pin_monitors[pin].push_back(monitor);
}

void Atmega32::removePinMonitor(int pin, PinMonitor* monitor)
{
    if ((pin < 0) || (pin >= MEGA32_PIN_COUNT))
        fail("No such pin in ATMEGA32 (no. %02x)", pin);
    
    vector<PinMonitor *>::iterator it = find(this->pin_monitors[pin].begin(), this->pin_monitors[pin].end(), monitor);
    
    this->pin_monitors[pin].erase(it);
}

void Atmega32::_triggerPinMonitors(int pin, int value)
{
    vector<PinMonitor *>::iterator it = this->pin_monitors[pin].begin();
    vector<PinMonitor *>::iterator end = this->pin_monitors[pin].end();

    while (it != end) {
        (*it)->onPinChanged(pin, value);
        it++;
    }
}


void Atmega32::_handleDataPortRead(uint8_t port, int8_t bit, uint8_t &value)
{
}

void Atmega32::_handleDataPortWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    int pin = pin_for_port(port);
    
    if (is_PIN_port(port)) {
        fail("Tried to write to PIN port");
    } else if (is_PORT_port(port)) {
        uint8_t mask = this->ports[port-1]; // read corresponding DDR register
        
        // copy data to PIN buffer (for pins set as output)
        this->ports[port-2] = (this->ports[port-2] & ~mask) | (value & mask);
        
        for (uint8_t i = 0; i < 8; i++)
            if (bit_is_set(mask, i) && (bit_is_set(value, i) != bit_is_set(prev_val, i)))
                this->_triggerPinMonitors(pin + i, bit_is_set(value, i));
    } else if (is_DDR_port(port)) {
        for (uint8_t i = 0; i < 8; i++)
            if (bit_is_set(value, i) && !bit_is_set(prev_val, i))
                this->_triggerPinMonitors(pin + i, bit_is_set(this->ports[port+1], i));
    }
}

void Atmega32::_twiInit()
{
    this->ports[PORT_TWBR] = 0x00;
    this->ports[PORT_TWSR] = 0xf8;
    this->ports[PORT_TWAR] = 0xfe;
    this->ports[PORT_TWDR] = 0xff;
    this->ports[PORT_TWCR] = 0x00;
    
    this->twi_has_floor = false;
    this->twi_start_just_sent = false;
}

void Atmega32::_twiHandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
}

void Atmega32::_twiHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    switch (port) {
        case PORT_TWSR:
            // reject writes to read-only bits
            value = (prev_val & 0xfc) + (value & 0x03);
            break;
        case PORT_TWCR:
            // My interpretation of the TWINT bit: write 1 to start the command,
            // or 0 to do nothing. You will not read what you last wrote, but
            // rather the current status (0=busy, 1=ready).
            // If I change another bit in the TWCR register and TWINT was previously
            // set to 1, should I interpret this as the go-signal for another command?
            // My guess is no.
            if (!bit_is_set(value, B_TWEN))
                this->twi_has_floor = false;
            if (bit_is_set(value, B_TWINT) && ((bit == B_TWINT) || (bit == -1))) {
                if (!bit_is_set(value, B_TWEN))
                    return;
                
                // All TWI operations appear to complete immediately (not realistic of course)
                set_bit(value, B_TWINT);
                
                // Note: TWSTA and TWSTO may be set simultaneously. The effect
                // is to send the STOP first
                if (bit_is_set(value, B_TWSTO)) {
                    this->_i2cSendStop();
                    this->_twiStatus(TWI_STATUS_IDLE);
                    clear_bit(value, B_TWSTO);
                    this->twi_has_floor = false;
                    this->twi_start_just_sent = false;
                    
                    if (!bit_is_set(value, B_TWSTA))
                        return;
                }
                
                // Send start
                if (bit_is_set(value, B_TWSTA)) {
                    this->_i2cSendStart();
                    this->_twiStatus(this->twi_has_floor ? TWI_STATUS_RESTART : TWI_STATUS_START);
                    this->twi_has_floor = true;
                    this->twi_start_just_sent = true;
                    return;
                }
                
                // Send address
                uint8_t data = this->ports[PORT_TWDR];
                if (!this->twi_has_floor)
                    fail("Tried to send/receive data on TWI without having the floor!");
 
                bool ack;
                if (this->twi_start_just_sent) {
                    this->twi_xmit_mode = !bit_is_set(data, 0);
                    ack = this->_i2cSendAddress((data >> 1) & 0x7f, twi_xmit_mode);
                    this->_twiStatus(twi_xmit_mode
                        ? (ack ? TWI_STATUS_W_SL_ACK : TWI_STATUS_W_SL_NACK)
                        : (ack ? TWI_STATUS_R_SL_ACK : TWI_STATUS_R_SL_NACK));
                    this->twi_start_just_sent = false;
                    return;
                }
                
                // Send/receive data
                if (this->twi_xmit_mode) {
                    ack = this->_i2cSendData(data);
                    this->_twiStatus(ack ? TWI_STATUS_W_DATA_ACK : TWI_STATUS_W_DATA_NACK);
                } else {
                    ack = this->_i2cQueryData();
                    this->_twiStatus(ack ? TWI_STATUS_R_DATA_ACK : TWI_STATUS_R_DATA_NACK);
                }
            }
    }
}

void Atmega32::_twiStatus(uint8_t status)
{
    this->ports[PORT_TWSR] = status | (this->ports[PORT_TWSR] & 0x03);
}

void Atmega32::i2cReceiveStart()
{
    fail("Slave behavior on I2C not supported for ATMEGA32");
}

void Atmega32::i2cReceiveStop()
{
    fail("Slave behavior on I2C not supported for ATMEGA32");
}

bool Atmega32::i2cReceiveAddress(uint8_t address, bool write)
{
    fail("Slave behavior on I2C not supported for ATMEGA32");
    
    return false;
}

bool Atmega32::i2cReceiveData(uint8_t data)
{
    if (this->twi_has_floor) {
        if (this->twi_xmit_mode)
            fail("ATMEGA32 Received I2C data while transmitting!");
        
        this->ports[PORT_TWDR] = data;
        
        printf("*** ATMEGA receives: %02x\n", data);
        return true;
    }
    
    fail("Slave behavior on I2C not supported for ATMEGA32");
    
    
    return false;
}

bool Atmega32::i2cQueryData(uint8_t &data)
{
    fail("Slave behavior on I2C not supported for ATMEGA32");
    
    return false;
}

void Atmega32::_spiInit()
{
    this->ports[PORT_SPCR] = 0x00;
    this->ports[PORT_SPSR] = 0x00;
    this->ports[PORT_SPDR] = 0xff;

    this->spi_stat_read = false;
}

void Atmega32::_spiHandleRead(uint8_t port, int8_t bit, uint8_t &value)
{
    switch (port) {
        case PORT_SPSR:
            if (bit_is_set(value, B_SPIF))
                this->spi_stat_read = true;
        case PORT_SPDR:
            if (this->spi_stat_read) {
                clear_bit(this->ports[PORT_SPSR], B_SPIF);
                this->spi_stat_read = false;
            }
    }
}

void Atmega32::_spiHandleWrite(uint8_t port, int8_t bit, uint8_t &value, uint8_t prev_val)
{
    switch (port) {
        case PORT_SPSR:
            // reject writes to read-only bits
            value = (prev_val & 0xfe) + (value & 0x01);
            break;
        case PORT_SPDR:
            if (!this->_spiIsEnabled())
                return;

            this->spi_stat_read = false;

            this->_spiSendData(value);

            set_bit(this->ports[PORT_SPSR], B_SPIF);
            break;
    }
}

bool Atmega32::_spiIsEnabled()
{
    return bit_is_set(this->ports[PORT_SPCR], B_SPE);
}

void Atmega32::spiSlaveSelect(bool select)
{
    if (select)
        fail("Slave behavior on SPI not supported for ATMEGA32");
}

bool Atmega32::spiReceiveData(uint8_t &data)
{
    fail("Slave behavior on SPI not supported for ATMEGA32");

    return false;
}
