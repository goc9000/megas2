#ifndef _H_ATMEGA32_DEFS_H
#define _H_ATMEGA32_DEFS_H

// Flash size is in 16-bit words
#define MEGA32_FLASH_SIZE  0x4000

// Others are in bytes
#define RAM_SIZE           0x0860
#define IO_BASE            0x0020   
#define SRAM_BASE          0x0060

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
#define PORT_ADCL         0x04
#define PORT_ADCH         0x05
#define PORT_ADCSRA       0x06
#define PORT_ADMUX        0x07
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
#define PORT_ICR1         0x26
#define PORT_ICR1L        0x26
#define PORT_ICR1H        0x27
#define PORT_OCR1B        0x28
#define PORT_OCR1BL       0x28
#define PORT_OCR1BH       0x29
#define PORT_OCR1A        0x2a
#define PORT_OCR1AL       0x2a
#define PORT_OCR1AH       0x2b
#define PORT_TCNT1        0x2c
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

// Bit values for TWCR
#define B_TWINT         7
#define B_TWSTA         5
#define B_TWSTO         4
#define B_TWEN          2
// Bit balues for ADCSRA
#define B_ADEN          7
#define B_ADSC          6
#define B_ADATE         5
#define B_ADIF          4
#define B_ADIE          3
// Bit values for SPSR 
#define B_SPIF          7
// Bit values for SPCR
#define B_SPIE          7
#define B_SPE           6
#define B_DORD          5
#define B_MSTR          4
// Bit values for TCCR1A
#define B_WGM11         1
#define B_WGM10         0
// Bit values for TCCR1B
#define B_WGM13         4
#define B_WGM12         3
// Bit values for TIMSK
#define B_OCIE2         7
#define B_TOIE2         6
#define B_TICIE1        5
#define B_OCIE1A        4
#define B_OCIE1B        3
#define B_TOIE1         2
#define B_OCIE0         1
#define B_TOIE0         0
// Bit values for TIFR
#define B_OCF2          7
#define B_TOV2          6
#define B_ICF1          5
#define B_OCF1A         4
#define B_OCF1B         3
#define B_TOV1          2
#define B_OCF0          1
#define B_TOV0          0

// SREG flags
#define FLAG_I          7
#define FLAG_T          6
#define FLAG_H          5
#define FLAG_S          4
#define FLAG_V          3
#define FLAG_N          2
#define FLAG_Z          1
#define FLAG_C          0

// IRQs

#define IRQ_RESET                1
#define IRQ_INT0                 2
#define IRQ_INT1                 3
#define IRQ_INT2                 4
#define IRQ_TIMER2_COMP          5
#define IRQ_TIMER2_OVF           6
#define IRQ_TIMER1_CAPT          7
#define IRQ_TIMER1_COMPA         8
#define IRQ_TIMER1_COMPB         9
#define IRQ_TIMER1_OVF          10
#define IRQ_TIMER0_COMP         11
#define IRQ_TIMER0_OVF          12
#define IRQ_SPI_STC             13
#define IRQ_USART_RXC           14
#define IRQ_USART_UDRE          15
#define IRQ_USART_TXC           16
#define IRQ_ADC                 17
#define IRQ_EE_RDY              18
#define IRQ_ANA_COMP            19
#define IRQ_TWI                 20
#define IRQ_SPM_RDY             21

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

static inline bool is_data_port(uint8_t port)
{
    return ((port >= PORT_PIND) && (port <= PORT_PORTA));
}

static inline bool is_twi_port(uint8_t port)
{
    return ((port >= PORT_TWBR) && (port <= PORT_TWDR)) || (port == PORT_TWCR);
}

static inline bool is_spi_port(uint8_t port)
{
    return ((port >= PORT_SPCR) && (port <= PORT_SPDR));
}

static inline bool is_timer_port(uint8_t port)
{
    return ((port >= PORT_ASSR) && (port <= PORT_TCCR1A))
        || (port == PORT_TCNT0) || (port == PORT_TCCR0) || (port == PORT_TIFR)
        || (port == PORT_TIMSK)  || (port == PORT_OCR0);
}

static inline bool is_adc_port(uint8_t port)
{
    return (port >= PORT_ADCL) && (port <= PORT_ADMUX);
}

#endif
