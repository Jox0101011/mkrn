/*
 * segments.h - gdt: selectors e formato dos descritores
 *
 * layout: null, kernel code, kernel data, user code, user data, tss.
 * o descritor de tss fica reservado (zerado) ate ter uma struct tss
 * de verdade pra apontar - isso e coisa de uma proxima etapa.
 */

#ifndef _MACHINE_SEGMENTS_H_
#define _MACHINE_SEGMENTS_H_

#include "../../../sys/types.h"


#define GSEL_NULL	0x00
#define GSEL_KCODE	0x08
#define GSEL_KDATA	0x10
#define GSEL_UCODE	(0x18 | 3)	/* rpl 3 */
#define GSEL_UDATA	(0x20 | 3)
#define GSEL_TSS	0x28

#define NGDT		6

/* descritor de segmento (code/data/tss), 8 bytes */
struct gdt_entry {
	uint16_t	limit_lo;
	uint16_t	base_lo;
	uint8_t		base_mid;
	uint8_t		access;
	uint8_t		limit_hi_flags;
	uint8_t		base_hi;
} __attribute__((packed));

/* operando do lgdt */
struct gdt_ptr {
	uint16_t	limit;
	uint32_t	base;
} __attribute__((packed));

/* byte de access (bit7 p, bits6-5 dpl, bit4 s, bit3 ex, bit2 dc,
   bit1 rw, bit0 accessed) */
#define GDT_A_PRESENT	0x80
#define GDT_A_RING(x)	(((x) & 3) << 5)
#define GDT_A_SEGMENT	0x10	/* 1 = code/data, 0 = sistema (ex.: tss) */
#define GDT_A_EXEC	0x08	/* 1 = code, 0 = data */
#define GDT_A_DC	0x04	/* conforming (code) / direction (data) */
#define GDT_A_RW	0x02	/* readable (code) / writable (data) */
#define GDT_A_ACCESSED	0x01

/* nibble alto de limit_hi_flags (bit7 g, bit6 d/b, bit5 l, bit4 avl) */
#define GDT_F_GRAN_4K	0x80
#define GDT_F_SZ_32	0x40

void gdt_init(void);

#endif /* !_MACHINE_SEGMENTS_H_ */
