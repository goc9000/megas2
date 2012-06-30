#ifndef _H_ATMEGA32_CORE_H
#define _H_ATMEGA32_CORE_H

#include <inttypes.h>

class Atmega32;

struct Atmega32Core {
    int pc;
    uint16_t flash[0x4000];
    uint8_t ram[0x0860];
    Atmega32 *master;
    int last_inst_pc;
};

void atmega32_core_init(Atmega32Core *core, Atmega32 *master);
void atmega32_core_step(Atmega32Core *core);

void set_flag(Atmega32Core *core, uint8_t bit, bool value);
bool get_flag(Atmega32Core *core, uint8_t bit);
void push_word(Atmega32Core *core, uint16_t value);

#endif

