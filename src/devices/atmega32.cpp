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
#include "atmega32.h"
#include "atmega32_defs.h"

using namespace std;

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

Atmega32::Atmega32()
{
    atmega32_core_init(&this->core, this);
    
    this->ports = this->core.ram + IO_BASE;
    
    this->setFrequency(8000);
    
    this->reset();
}

void Atmega32::setFrequency(unsigned frequency_khz)
{
    this->frequency_khz = frequency_khz;
    this->clock_period = ns_to_sim_time(1000000/frequency_khz);
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
        if (phdr.p_paddr + phdr.p_filesz > 2*FLASH_SIZE)
            fail("Segment #%d too long (addr=%04x, length=%d bytes)",
                i, (int)phdr.p_paddr, (int)phdr.p_filesz);

        lseek(fd, phdr.p_offset, SEEK_SET);
        if (read(fd, this->core.flash + phdr.p_paddr/2, phdr.p_filesz) != (int)phdr.p_filesz)
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
    memset(this->core.ram, 0, RAM_SIZE);
    this->core.pc = 0;
    
    this->_twiInit();
    this->_spiInit();
}

void Atmega32::setSimulationTime(sim_time_t time)
{
    sim_time_t delta = time - this->sim_time;
    
    this->sim_time = time;
    this->next_fetch_time += delta;
}

void Atmega32::act()
{
    this->sim_time = this->next_fetch_time;
    
    this->step();
    
    this->next_fetch_time = this->sim_time + this->clock_period;
}

sim_time_t Atmega32::nextEventTime()
{
    return this->next_fetch_time;
}

void Atmega32::step()
{
    atmega32_core_step(&this->core);
}

void Atmega32::_onPortRead(uint8_t port, int8_t bit, uint8_t &value)
{
    if (port >= REG16_SP-IO_BASE)
        return;
    
    if (is_twi_port(port)) {
        this->_twiHandleRead(port, bit, value);
    } else if (is_spi_port(port)) {
        this->_spiHandleRead(port, bit, value);
    } else if (is_data_port(port)) {
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
    if (port >= REG16_SP-IO_BASE)
        return;
        
    if (is_twi_port(port)) {
        this->_twiHandleWrite(port, bit, value, prev_val);
    } else if (is_spi_port(port)) {
        this->_spiHandleWrite(port, bit, value, prev_val);
    } else if (is_data_port(port)) {
        this->_handleDataPortWrite(port, bit, value, prev_val);
    } else {
        if (bit < 0) {
            printf("%04x: OUT %s(%02x) <- %02x (was %02x)\n", this->core.last_inst_pc * 2, PORT_NAMES[port], port,
                value, prev_val);
        } else {
            printf("%04x: OUT %s(%02x).%d <- %d (was %d)\n", this->core.last_inst_pc * 2, PORT_NAMES[port], port, bit,
                (value >> bit) & 1, (prev_val >> bit) & 1);
        }
    }
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

uint8_t Atmega32::_read16BitReg(uint8_t reg)
{
    return (this->core.ram[reg+1] << 8) + this->core.ram[reg];
}

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
        this->_read16BitReg(REG16_X), this->_read16BitReg(REG16_Y),
        this->_read16BitReg(REG16_Z), this->_read16BitReg(REG16_SP),
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
