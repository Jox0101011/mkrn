/*
 * vga.c - console de texto vga (0xb8000), 80x25
 */


#include "../sys/types.h"
#include "include/machine/cpufunc.h"
#include "../sys/cons.h"

#define VGA_ADDR	0xb8000
#define VGA_COLS	80
#define VGA_ROWS	25
#define VGA_ATTR	0x07	/* cinza sobre preto */
#define VGA_ATTR_LOG	0xf0	/* preto sobre branco: linhas de log do kernel */

static uint16_t *const vga = (uint16_t *)VGA_ADDR;
static unsigned vga_row, vga_col;

static void
vga_setcursor(void)
{
	uint16_t pos = vga_row * VGA_COLS + vga_col;

	outb(0x3d4, 0x0f);
	outb(0x3d5, pos & 0xff);
	outb(0x3d4, 0x0e);
	outb(0x3d5, (pos >> 8) & 0xff);
}

/*
 * o attribute controller trata o bit 3 do nibble de fundo como
 * "pisca" por padrao, nao como intensidade - sem isso, VGA_ATTR_LOG
 * (fundo 0xf) pisca em vez de dar branco solido. indice 0x10 e o
 * "mode control"; o bit 5 no byte de indice so mantem o video ligado
 * durante o acesso (senao a tela pisca uma vez). port 0x3da antes
 * reseta o flip-flop indice/dado do attribute controller.
 */
static void
vga_enable_bg_intensity(void)
{
	uint8_t val;

	inb(0x3da);
	outb(0x3c0, 0x10 | 0x20);
	val = inb(0x3c1);
	outb(0x3c0, val & ~0x08);
}

static void
vga_scroll(void)
{
	unsigned i;

	for (i = 0; i < VGA_COLS * (VGA_ROWS - 1); i++)
		vga[i] = vga[i + VGA_COLS];
	for (i = 0; i < VGA_COLS; i++)
		vga[VGA_COLS * (VGA_ROWS - 1) + i] = (VGA_ATTR << 8) | ' ';
	vga_row = VGA_ROWS - 1;
}

void
vga_init(void)
{
	unsigned i;

	vga_enable_bg_intensity();

	for (i = 0; i < VGA_COLS * VGA_ROWS; i++)
		vga[i] = (VGA_ATTR << 8) | ' ';
	vga_row = 0;
	vga_col = 0;
	vga_setcursor();
}

void
vga_putc(int c, int log)
{
	uint16_t attr = log ? VGA_ATTR_LOG : VGA_ATTR;

	if (c == '\n') {
		vga_col = 0;
		vga_row++;
	} else {
		vga[vga_row * VGA_COLS + vga_col] = (attr << 8) | (uint8_t)c;
		if (++vga_col == VGA_COLS) {
			vga_col = 0;
			vga_row++;
		}
	}

	if (vga_row == VGA_ROWS)
		vga_scroll();

	vga_setcursor();
}
