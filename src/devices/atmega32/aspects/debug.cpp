#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctype.h>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>

#include "utils/bit_macros.h"
#include "utils/fail.h"
#include "devices/atmega32/atmega32.h"
#include "devices/atmega32/defs.h"

using namespace std;

void Atmega32::_dumpRegisters()
{
    char const *SREG_REP = "ithsvnzc";
    char sreg_rep[9];
    
    printf("--- Registers ---\n");
    
    for (int i=0; i<IO_BASE; i++) {
        printf("%c%02d", i ? ' ' : 'r', i);
    }
    printf("\n");
    for (int i=0; i<IO_BASE; i++) {
        printf(" %02x", this->core.ram[i]);
    }
    printf("\n");
    
    uint8_t sreg = this->core.ram[REG_SREG];
    for (int i=0; i<8; i++)
        sreg_rep[i] = bit_is_set(sreg, 7-i) ? toupper(SREG_REP[i]) : SREG_REP[i];
    sreg_rep[8] = 0;

    printf("X=%04x Y=%04x Z=%04x SP=%04x SREG=%02x (%s)\n",
        this->_get16BitReg(REG16_X), this->_get16BitReg(REG16_Y),
        this->_get16BitReg(REG16_Z), this->_get16BitReg(REG16_SP),
        sreg, sreg_rep);
    
    printf("Last fetch @PC=%04x (in bytes: %04x)\n", this->core.last_inst_pc, 2*this->core.last_inst_pc);
}

void Atmega32::_dumpSram()
{
    const int BYTES_PER_BLOCK = 32;
    int size = RAM_SIZE - SRAM_BASE;
    int blocks = (size + BYTES_PER_BLOCK-1) / BYTES_PER_BLOCK;
    
    printf("--- SRAM contents ---\n");
    
    printf("     ");
    for (int i = 0; i < BYTES_PER_BLOCK; i++)
        printf("%c%02x", i ? ' ' : '+', i);
    printf("\n");
    
    for (int i = 0; i < blocks; i++) {
        int addr = i * BYTES_PER_BLOCK;
        printf("%04x:", addr);
        
        int count = min(size - addr, BYTES_PER_BLOCK);
        for (int j = 0; j < count; j++)
            printf(" %02x", this->core.ram[SRAM_BASE + addr + j]);
        for (int j = BYTES_PER_BLOCK; j < count; j++)
            printf("   ");
        printf(" | ");
        for (int j = 0; j < count; j++) {
            uint8_t byte = this->core.ram[SRAM_BASE + addr + j];
            printf("%c", ((byte >= 32) && (byte <= 126)) ? byte : '.');
        }
        printf("\n");
    }
}

void Atmega32::_dumpPortRead(const char *header, uint8_t port, int8_t bit, uint8_t &value)
{
    char buffer[1024];
    int len = 0;
    
    len += sprintf(buffer + len, "%04x: ", this->core.last_inst_pc * 2);
    
    if (header) {
        len += sprintf(buffer + len, "%s: ", header);
    }
    
    len += sprintf(buffer + len, "READ from %s", PORT_NAMES[port]);
    if (bit != -1) {
        len += sprintf(buffer + len, ":%d", bit);
    }
    
    len += sprintf(buffer + len, " = ");
    
    if (bit == -1) {
        len += sprintf(buffer + len, "%02x", value);
    } else {
        len += sprintf(buffer + len, "%d (full: %02x)", bit_is_set(value, bit), value);
    }
    
    buffer[len] = 0;
    printf("%s\n", buffer);
}

void Atmega32::_dumpPortWrite(const char *header, uint8_t port, int8_t bit, uint8_t value, uint8_t prev_val, uint8_t cleared)
{
    char buffer[1024];
    int len = 0;
    
    len += sprintf(buffer + len, "%04x: ", this->core.last_inst_pc * 2);
    
    if (header) {
        len += sprintf(buffer + len, "%s: ", header);
    }
    
    len += sprintf(buffer + len, "WRITE to %s", PORT_NAMES[port]);
    if (bit != -1) {
        len += sprintf(buffer + len, ":%d", bit);
    }
    
    len += sprintf(buffer + len, " : ");
    
    if (bit == -1) {
        len += sprintf(buffer + len, "%02x->%02x", prev_val, value);
    } else {
        len += sprintf(buffer + len, "%d->%d (full: %02x->%02x)",
            bit_is_set(prev_val, bit), bit_is_set(value, bit), prev_val, value);
    }
    
    if (cleared) {
        len += sprintf(buffer + len, " cleared:%02x", cleared);
    }
    
    buffer[len] = 0;
    printf("%s\n", buffer);
}
