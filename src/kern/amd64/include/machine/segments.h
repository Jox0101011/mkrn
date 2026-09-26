/*
 * segments.h - gdt: selectors e formato dos descritores
 *
 * layout: null, kernel code, kernel data, user code, user data, tss.
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

/* tipo (bits0-3 do access) de um descritor de sistema com
   GDT_A_SEGMENT=0 - so usamos esse, tss disponivel de 32 bits */
#define GDT_A_TSS32	0x09

/*
 * task state segment (32 bits) - layout fixo da intel, so ss0/esp0
 * importam de verdade aqui: nao usamos troca de task por hardware
 * (ver o comentario grande em amd64/gdt.c), so precisamos que ss0:esp0
 * estejam certos pra quando uma interrupcao pegar a cpu em ring3 e
 * subir sozinha pra ring0 - o resto dos campos so existe porque o
 * formato e fixo, fica tudo zerado.
 */
struct tss {
	uint32_t	prev_task;
	uint32_t	esp0;
	uint32_t	ss0;
	uint32_t	esp1;
	uint32_t	ss1;
	uint32_t	esp2;
	uint32_t	ss2;
	uint32_t	cr3;
	uint32_t	eip;
	uint32_t	eflags;
	uint32_t	eax, ecx, edx, ebx;
	uint32_t	esp, ebp, esi, edi;
	uint32_t	es, cs, ss, ds, fs, gs;
	uint32_t	ldt;
	uint16_t	trap;
	uint16_t	iomap_base;
} __attribute__((packed));

void tss_set_kstack(uint32_t esp0);	/* amd64/gdt.c - chamado a cada troca de thread */

void gdt_init(void);

#endif /* !_MACHINE_SEGMENTS_H_ */
