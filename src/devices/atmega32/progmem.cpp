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
#include "gelf.h"
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

    Elf *elf = this->_openElf(filename);
    this->_processElfSections(elf);
    this->_loadProgramSegments(elf);    
    elf_end(elf);
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

void ProgMem::_processElfSections(Elf *elf)
{
    Elf_Scn *section = NULL;
    while ((section = elf_nextscn(elf, section))) {
        GElf_Shdr shdr;
        gelf_getshdr(section, &shdr);

        if (shdr.sh_type == SHT_SYMTAB) {
            this->_processElfSymbolTable(elf, section);
        }
    }
}

void ProgMem::_processElfSymbolTable(Elf *elf, Elf_Scn *section)
{
    GElf_Shdr shdr;
    gelf_getshdr(section, &shdr);

    Elf_Data *data = NULL;
    while ((data = elf_getdata(section, data))) {
        int count = shdr.sh_size / shdr.sh_entsize;

        for (int i = 0; i < count; i++) {
            GElf_Sym sym;
            gelf_getsym(data, i, &sym);

            uint8_t sym_type = sym.st_info & 0xf;
            char *sym_name = elf_strptr(elf, shdr.sh_link, sym.st_name);

            if (sym_type == STT_FUNC) {
                this->flash_syms.push_back(Symbol(sym_name, sym.st_value, sym.st_size, false));
            } else if (sym_type == STT_OBJECT) {
                if (sym.st_value >= 0x100000) {
                    this->ram_syms.push_back(Symbol(sym_name, sym.st_value & 0xfffff, sym.st_size, true));
                } else {
                    this->flash_syms.push_back(Symbol(sym_name, sym.st_value, sym.st_size, true));
                }
            }
        }
    }

    sort(this->flash_syms.begin(), this->flash_syms.end());
    sort(this->ram_syms.begin(), this->ram_syms.end());
    this->_computeSymbolCover();
}

Elf * ProgMem::_openElf(const char *filename)
{
    int fd, temp;
    Elf *elf;
    GElf_Ehdr ehdr;

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

    return elf;
}

void ProgMem::_loadProgramSegments(Elf *elf)
{
    bool exec_found = false;

    size_t file_size;
    char *raw_data = elf_rawfile(elf, &file_size);

    size_t count;
    elf_getphdrnum(elf, &count);
    for (unsigned int i = 0; i < count; i++) {
        GElf_Phdr phdr;
        gelf_getphdr(elf, i, &phdr);

        if (phdr.p_type != PT_LOAD) continue;
        if (!phdr.p_filesz) continue; // ignore .bss

        if (phdr.p_paddr & 1)
            fail("ELF segment #%d starts at odd address (%04x)", i, (int)phdr.p_paddr);
        if (phdr.p_paddr + phdr.p_filesz > 2*this->flash_size)
            fail("ELF segment #%d too long (addr=%04x, length=%d bytes)",
                i, (int)phdr.p_paddr, (int)phdr.p_filesz);

        if (phdr.p_offset + phdr.p_filesz >= file_size)
            fail("ELF segment #%d corrupt (extends past end of file)", i);

        memcpy(this->flash + phdr.p_paddr/2, raw_data + phdr.p_offset, phdr.p_filesz);
        
        exec_found |= (phdr.p_flags & PF_X);
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
