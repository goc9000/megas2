#ifndef _H_ATMEGA32_DEFS_H
#define _H_ATMEGA32_DEFS_H

// Flash size is in 16-bit words
#define FLASH_SIZE      0x4000

// Others are in bytes
#define RAM_SIZE        0x0860
#define IO_BASE         0x0020   
#define SRAM_BASE       0x0060

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
#define REG16_X           0x1a
#define REG16_Y           0x1c
#define REG16_Z           0x1e
#define REG16_SP          0x5d
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
#define PORT_ASSR         0x22
#define PORT_OCR2         0x23
#define PORT_TCNT2        0x24
#define PORT_TCCR2        0x25
#define PORT_ICR1L        0x26
#define PORT_ICR1H        0x27
#define PORT_OCR1BL       0x28
#define PORT_OCR1BH       0x29
#define PORT_OCR1AL       0x2a
#define PORT_OCR1AH       0x2b
#define PORT_TCNT1L       0x2c
#define PORT_TCNT1H       0x2d
#define PORT_TCCR1B       0x2e
#define PORT_TCCR1A       0x2f
#define PORT_TCNT0        0x32
#define PORT_TCCR0        0x33
#define PORT_TWCR         0x36
#define PORT_TIFR         0x38
#define PORT_TIMSK        0x39
#define PORT_OCR0         0x3c

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
#define TWI_STATUS_START        0x08
#define TWI_STATUS_RESTART      0x10
#define TWI_STATUS_W_SL_ACK     0x18
#define TWI_STATUS_W_SL_NACK    0x20
#define TWI_STATUS_W_DATA_ACK   0x28
#define TWI_STATUS_W_DATA_NACK  0x30
#define TWI_STATUS_R_SL_ACK     0x40
#define TWI_STATUS_R_SL_NACK    0x48
#define TWI_STATUS_R_DATA_ACK   0x50
#define TWI_STATUS_R_DATA_NACK  0x58
#define TWI_STATUS_IDLE         0xf8

#endif
