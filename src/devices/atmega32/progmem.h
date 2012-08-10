#ifndef _H_PROGMEM_H
#define _H_PROGMEM_H

#include <inttypes.h>

#define MAX_FLASH_SIZE 65536

class ProgMem {
public:
    ProgMem(unsigned int flash_size);
    void clear();
    void loadElf(const char *filename);
    
    uint8_t readByte(unsigned int byte_addr);

    uint16_t flash[MAX_FLASH_SIZE];
    const unsigned int flash_size;
};

#endif
