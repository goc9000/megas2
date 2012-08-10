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
}

void ProgMem::loadElf(const char *filename)
{
    int fd, temp;
    Elf *elf;
    GElf_Ehdr ehdr;
    size_t count;
    bool found = false;

    this->clear();

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
        if (phdr.p_paddr + phdr.p_filesz > 2*this->flash_size)
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

uint8_t ProgMem::readByte(unsigned int byte_addr)
{
    if (byte_addr >= 2*this->flash_size)
        fail("LPM from invalid program memory address (%04x)", byte_addr);
    
    return (byte_addr & 1) ?
        high_byte(this->flash[byte_addr >> 1]) :
        low_byte(this->flash[byte_addr >> 1]);
}
