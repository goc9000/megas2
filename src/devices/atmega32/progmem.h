#ifndef _H_PROGMEM_H
#define _H_PROGMEM_H

#include <vector>
#include <inttypes.h>

#include <elfio/elfio.hpp>

#include "symbol.h"

using namespace std;

#define PROGMEM_MAX_FLASH_SIZE 65536
#define PROGMEM_MAX_RAM_SIZE    4096

class ProgMem {
public:
    ProgMem(unsigned int flash_size);
    void clear();
    void loadElf(const char *filename);
    
    uint8_t readByte(unsigned int byte_addr);
    Symbol *flashSymbolAt(int flash_byte_addr);
    Symbol *ramSymbolAt(int ram_vaddr);
    void dumpFlashSyms();
    void dumpRamSyms();

    uint16_t flash[PROGMEM_MAX_FLASH_SIZE];
    const unsigned int flash_size;

    vector<Symbol> flash_syms;
    vector<Symbol> ram_syms;
private:
    void _processElfSections(ELFIO::elfio& elf);
    void _processElfSymbolTable(ELFIO::elfio& elf, ELFIO::section* section);
    void _loadProgramSegments(ELFIO::elfio& elf);
    void _computeSymbolCover();

    Symbol* sym_at[2*PROGMEM_MAX_FLASH_SIZE];
    Symbol* ram_sym_at[PROGMEM_MAX_RAM_SIZE];
};

#endif
