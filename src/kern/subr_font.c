/*
 * subr_font.c - renderer de texto em cima de um framebuffer generico
 */

#include <stdint.h>

#include "sys/font.h"
#include "sys/font8x8.h"

static void
putpixel(struct fb *fb, uint32_t x, uint32_t y, uint32_t color)
{
	uint8_t *p;

	if (x >= fb->width || y >= fb->height)
		return;

	p = fb->addr + (uint32_t)y * fb->pitch + x * (fb->bpp / 8);

	switch (fb->bpp) {
	case 8:
		p[0] = (uint8_t)color;
		break;
	case 16:
		p[0] = (uint8_t)color;
		p[1] = (uint8_t)(color >> 8);
		break;
	case 24:
		p[0] = (uint8_t)color;
		p[1] = (uint8_t)(color >> 8);
		p[2] = (uint8_t)(color >> 16);
		break;
	case 32:
	default:
		p[0] = (uint8_t)color;
		p[1] = (uint8_t)(color >> 8);
		p[2] = (uint8_t)(color >> 16);
		p[3] = (uint8_t)(color >> 24);
		break;
	}
}

void
font_putc(struct fb *fb, uint32_t x, uint32_t y, unsigned char c,
    uint32_t fg, uint32_t bg)
{
	int row, col;

	for (row = 0; row < FONT_HEIGHT; row++) {
		unsigned char bits = font8x8[c][row];

		for (col = 0; col < FONT_WIDTH; col++) {
			/* msb = pixel da esquerda (comentario original da fonte) */
			int on = bits & (0x80 >> col);
			putpixel(fb, x + (uint32_t)col, y + (uint32_t)row,
			    on ? fg : bg);
		}
	}
}

void
font_puts(struct fb *fb, uint32_t x, uint32_t y, const char *s,
    uint32_t fg, uint32_t bg)
{
	uint32_t ox = x;

	for (; *s != '\0'; s++) {
		if (*s == '\n') {
			x = ox;
			y += FONT_HEIGHT;
			continue;
		}

		font_putc(fb, x, y, (unsigned char)*s, fg, bg);
		x += FONT_WIDTH;
	}
}

/* framebuffer de teste: 32bpp, exatamente uma celula de caractere.
   estatico de proposito - sem isso, o selftest nao rodaria antes do
   pmm/vmm existirem */
static uint32_t selftest_mem[FONT_WIDTH * FONT_HEIGHT];

int
font_selftest(void)
{
	struct fb fb;
	unsigned i;
	int lit;

	fb.addr = (uint8_t *)selftest_mem;
	fb.width = FONT_WIDTH;
	fb.height = FONT_HEIGHT;
	fb.pitch = FONT_WIDTH * sizeof(uint32_t);
	fb.bpp = 32;

	for (i = 0; i < FONT_WIDTH * FONT_HEIGHT; i++)
		selftest_mem[i] = 0;

	font_putc(&fb, 0, 0, 'A', 0xffffffffu, 0x00000000u);

	lit = 0;
	for (i = 0; i < FONT_WIDTH * FONT_HEIGHT; i++)
		if (selftest_mem[i] == 0xffffffffu)
			lit++;

	return lit > 0;
}
