/*
 * fb.c - console de texto em cima do framebuffer linear (multiboot2),
 * desenhando cada letra com o renderer de sys/font.h
 *
 * so entende framebuffer_type == rgb direto (ver machine/multiboot.h)
 * com 15/16/24/32 bits por pixel - qualquer outra coisa, fb_init()
 * devolve 0 e amd64/cons.c continua no vga de texto.
 */


#include "../sys/types.h"
#include "../sys/cons.h"
#include "../sys/font.h"
#include "../sys/libkern.h"

static struct fb md_fb;
static struct cons_fb cfb;
static unsigned fb_cols, fb_rows;
static unsigned fb_col, fb_row;

/* empacota r/g/b (0-255 cada) pro formato de pixel que o fb pediu,
   usando a posicao/tamanho de campo de cada canal (mesma ideia de
   uma vbe/vesa direct color): trunca cada componente pro tamanho do
   campo e desloca pra posicao certa. com r=g=b=0 (preto) ou 255
   (branco) isso da 0x0 e "todos os bits do campo em 1"
   respectivamente, direto, sem se importar com o layout exato */
static uint32_t
fb_pack(uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t red, green, blue;

	red   = (uint32_t)(r >> (8 - cfb.red_size))   << cfb.red_pos;
	green = (uint32_t)(g >> (8 - cfb.green_size)) << cfb.green_pos;
	blue  = (uint32_t)(b >> (8 - cfb.blue_size))  << cfb.blue_pos;

	return red | green | blue;
}

static void
fb_scroll(void)
{
	uint8_t *base = md_fb.addr;
	uint32_t row_bytes = (uint32_t)FONT_HEIGHT * cfb.pitch;

	memmove(base, base + row_bytes, (uint32_t)(fb_rows - 1) * row_bytes);
	/* preto e sempre byte 0, nao importa o layout de cor -
	   fb_pack(0,0,0) tambem daria 0, isso so evita o calculo */
	memset(base + (uint32_t)(fb_rows - 1) * row_bytes, 0, row_bytes);
	fb_row = fb_rows - 1;
}

void
fb_putc(int c, int log)
{
	uint32_t fg, bg;

	if (log) {
		fg = fb_pack(0, 0, 0);
		bg = fb_pack(255, 255, 255);
	} else {
		fg = fb_pack(170, 170, 170);	/* cinza claro, mesmo espirito do vga */
		bg = fb_pack(0, 0, 0);
	}

	if (c == '\n') {
		fb_col = 0;
		fb_row++;
	} else {
		font_putc(&md_fb, fb_col * FONT_WIDTH, fb_row * FONT_HEIGHT,
		    (unsigned char)c, fg, bg);
		if (++fb_col == fb_cols) {
			fb_col = 0;
			fb_row++;
		}
	}

	if (fb_row == fb_rows)
		fb_scroll();
}

int
fb_init(struct cons_fb *fb)
{
	if (fb->bpp != 15 && fb->bpp != 16 && fb->bpp != 24 && fb->bpp != 32)
		return 0;
	if (fb->width < FONT_WIDTH || fb->height < FONT_HEIGHT || fb->pitch == 0)
		return 0;

	cfb = *fb;

	md_fb.addr = (uint8_t *)(uintptr_t)cfb.addr;
	md_fb.width = cfb.width;
	md_fb.height = cfb.height;
	md_fb.pitch = cfb.pitch;
	md_fb.bpp = cfb.bpp;

	fb_cols = cfb.width / FONT_WIDTH;
	fb_rows = cfb.height / FONT_HEIGHT;
	fb_col = fb_row = 0;

	memset(md_fb.addr, 0, (uint32_t)cfb.pitch * cfb.height);

	return 1;
}
