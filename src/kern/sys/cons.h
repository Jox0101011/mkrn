/*
 * cons.h - console do kernel
 *
 * kputc() vai pro framebuffer linear se cons_fb_init() conseguiu
 * ligar um (amd64/fb.c), ou pro vga de texto por padrao/fallback
 * (amd64/vga.c) - amd64/cons.c decide qual dos dois esta ativo.
 */

#ifndef _SYS_CONS_H_
#define _SYS_CONS_H_

#include "types.h"

/* framebuffer linear ja mapeado (addr e endereco virtual valido) -
   quem descobre isso (amd64/main.c, a partir da tag de framebuffer
   do multiboot2) preenche e chama cons_fb_init() */
struct cons_fb {
	uint32_t	addr;
	uint32_t	pitch, width, height;
	uint8_t		bpp;
	uint8_t		red_pos, red_size;
	uint8_t		green_pos, green_size;
	uint8_t		blue_pos, blue_size;
};

void vga_init(void);
int  cons_fb_init(struct cons_fb *fb);	/* 1 = trocou pro fb, 0 = bpp nao suportado, continua no vga */
void kputc(int c);

/* liga/desliga o destaque de log (preto sobre branco) nos proximos
   kputc() - so nas celulas realmente escritas, nao limpa a tela nem
   preenche o resto da linha */
void cons_log(int on);

#endif /* !_SYS_CONS_H_ */
