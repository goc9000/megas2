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
#include "progmem.h"

using namespace std;

ProgMem::ProgMem(unsigned int flash_size) : flash_size(flash_size)
{
    this->clear();
}

void ProgMem::clear()
{
    memset(this->flash, 0xFF, 2*this->flash_size);
    this->flash_syms.clear();
    this->ram_syms.clear();
}

void ProgMem::loadElf(const char *filename)
{
    this->clear();
    
    ELFIO::elfio elf;
    
    if (!elf.load(filename))
        fail("Cannot open file '%s' as ELF", filename);
    if (elf.get_class() != ELFCLASS32)
        fail("Invalid ELF class (expected ELFCLASS32)");

    this->_processElfSections(elf);
    this->_loadProgramSegments(elf);
}

uint8_t ProgMem::readByte(unsigned int byte_addr)
{
    if (byte_addr >= 2*this->flash_size)
        fail("LPM from invalid program memory address (%04x)", byte_addr);
    
    return (byte_addr & 1) ?
        high_byte(this->flash[byte_addr >> 1]) :
        low_byte(this->flash[byte_addr >> 1]);
}

Symbol * ProgMem::flashSymbolAt(int flash_byte_addr)
{
    return this->sym_at[flash_byte_addr];
}

Symbol * ProgMem::ramSymbolAt(int ram_vaddr)
{
    return this->ram_sym_at[ram_vaddr];
}

void ProgMem::dumpFlashSyms()
{
    for (vector<Symbol>::iterator it = this->flash_syms.begin(); it != this->flash_syms.end(); it++) {
        it->dump();
    }
}

void ProgMem::dumpRamSyms()
{
    for (vector<Symbol>::iterator it = this->ram_syms.begin(); it != this->ram_syms.end(); it++) {
        it->dump();
    }
}

void ProgMem::_processElfSections(ELFIO::elfio& elf)
{
    for (unsigned int i = 0; i < elf.sections.size(); i++) {
        if (elf.sections[i]->get_type() == SHT_SYMTAB)
            this->_processElfSymbolTable(elf, elf.sections[i]);
    }
}

void ProgMem::_processElfSymbolTable(ELFIO::elfio& elf, ELFIO::section* section)
{
    const ELFIO::symbol_section_accessor symbols(elf, section);
    
    for (unsigned int i = 0; i < symbols.get_symbols_num(); i++) {
        std::string name;
        ELFIO::Elf64_Addr value;
        ELFIO::Elf_Xword size;
        unsigned char bind;
        unsigned char type;
        ELFIO::Elf_Half section_index;
        unsigned char other;
        
        symbols.get_symbol(i, name, value, size, bind, type, section_index, other);
        
        switch (type) {
            case STT_FUNC:
                this->flash_syms.push_back(Symbol(name.c_str(), value, size, false));
                break;
            case STT_OBJECT:
                if (value >= 0x100000) {
                    this->ram_syms.push_back(Symbol(name.c_str(), value & 0xfffff, size, true));
                } else {
                    this->flash_syms.push_back(Symbol(name.c_str(), value, size, true));
                }
                break;
        }
    }
    
    sort(this->flash_syms.begin(), this->flash_syms.end());
    sort(this->ram_syms.begin(), this->ram_syms.end());
    this->_computeSymbolCover();
}

void ProgMem::_loadProgramSegments(ELFIO::elfio& elf)
{
    bool exec_found = false;

    for (unsigned int i = 0; i < elf.segments.size(); i++) {
        const ELFIO::segment* seg = elf.segments[i];
        
        unsigned int ph_addr = seg->get_physical_address();
        unsigned int seg_size = seg->get_file_size();
        
        if (seg->get_type() != PT_LOAD) continue;
        if (seg_size == 0) continue; // ignore .bss
        
        if (ph_addr & 1)
            fail("ELF segment #%d starts at odd address (%04x)", i, ph_addr);
        if (ph_addr + seg_size > 2*this->flash_size)
            fail("ELF segment #%d too long (addr=%04x, length=%d bytes)",
                i, ph_addr, seg_size);
        
        memcpy(this->flash + ph_addr/2, seg->get_data(), seg_size);
        
        exec_found |= (seg->get_flags() & PF_X);
    }
    
    if (!exec_found)
        fail("No executable section found");
}

void ProgMem::_computeSymbolCover()
{
    for (int i = 0; i < 2*PROGMEM_MAX_FLASH_SIZE; i++)
        this->sym_at[i] = NULL;

    for (vector<Symbol>::iterator it = this->flash_syms.begin(); it != this->flash_syms.end(); it++) {
        for (int i = it->address; i < it->address + it->length; i++)
            this->sym_at[i] = &(*it);
    }
        
    for (int i = 0; i < PROGMEM_MAX_RAM_SIZE; i++)
        this->ram_sym_at[i] = NULL;

    for (vector<Symbol>::iterator it = this->ram_syms.begin(); it != this->ram_syms.end(); it++) {
        for (int i = it->address; i < it->address + it->length; i++)
            this->ram_sym_at[i] = &(*it);
    }
}
