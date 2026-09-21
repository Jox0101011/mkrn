/*
 * font.h - renderer de texto em cima de um framebuffer generico
 *
 * o "struct fb" aqui nao sabe nada de vga, vbe ou uefi gop - e so
 * um bloco de pixels lineares (endereco + largura/altura + pitch +
 * bits por pixel). quem descobre o framebuffer de verdade (hoje
 * nenhum; amanha pode ser um modo vbe pedido via multiboot, ou uma
 * gop do uefi) so precisa preencher essa struct e chamar font_putc/
 * font_puts - o desenho do glifo e sempre o mesmo codigo.
 *
 * e por isso que isso entra cedo: quando o kernel ganhar um
 * framebuffer de verdade, o renderer ja estara pronto e testado,
 * sem precisar escrever nem debugar nada disso na hora.
 */

#ifndef _SYS_FONT_H_
#define _SYS_FONT_H_

#include "types.h"


#define FONT_WIDTH	8
#define FONT_HEIGHT	8

struct fb {
	uint8_t		*addr;		/* primeiro pixel */
	uint32_t	width, height;	/* em pixels */
	uint32_t	pitch;		/* bytes por linha (pode ter padding) */
	uint8_t		bpp;		/* bits por pixel: 8, 16, 24 ou 32 */
};

/* fg/bg ja vem no formato de pixel que o bpp do fb espera (quem
   chama e que sabe empacotar rgb pro bpp certo - este modulo nao
   entende de espaco de cor, so poe o valor que mandaram) */
void font_putc(struct fb *fb, uint32_t x, uint32_t y, unsigned char c,
    uint32_t fg, uint32_t bg);
void font_puts(struct fb *fb, uint32_t x, uint32_t y, const char *s,
    uint32_t fg, uint32_t bg);

/* desenha um glifo num framebuffer estatico interno e confere que
   pelo menos um pixel acendeu - da pra chamar bem cedo no boot, sem
   pmm/vmm/heap nem framebuffer de verdade nenhum */
int font_selftest(void);

#endif /* !_SYS_FONT_H_ */
