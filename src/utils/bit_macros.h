#ifndef _H_BIT_MACROS_H
#define _H_BIT_MACROS_H

#define _BV(bit) (1L << (bit))
#define low_byte(value) ((value) & 0xff)
#define high_byte(value) (((value) >> 8) & 0xff)
#define bit_is_set(value,bit) (((value) >> (bit)) & 1)
#define bit_was_set(before,after,bit) (((~(before) & (after)) >> (bit)) & 1)
#define bit_was_cleared(before,after,bit) ((((before) & ~(after)) >> (bit)) & 1)
#define set_bit(lval,bit) do { lval |= (1 << (bit)); } while (0)
#define clear_bit(lval,bit) do { lval &= ~(1 << (bit)); } while (0)
#define chg_bit(lval,bit,value) do { if (value) { set_bit(lval,bit); } else { clear_bit(lval,bit); }} while (0)

#endif
