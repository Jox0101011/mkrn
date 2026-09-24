/*
 * cons.c - despacha kputc() pro console ativo (framebuffer linear ou
 * vga de texto), e guarda o destaque de log que os dois entendem
 *
 * so existe um kputc() no kernel inteiro (e o que subr_prf.c chama);
 * vga_putc()/fb_putc() sao implementacao, nao aparecem em cons.h -
 * so esse arquivo fala com os dois.
 */

#include "../sys/cons.h"

void vga_putc(int c, int log);		/* amd64/vga.c */
int  fb_init(struct cons_fb *fb);	/* amd64/fb.c */
void fb_putc(int c, int log);		/* amd64/fb.c */

static int use_fb;
static int log_on;

void
cons_log(int on)
{
	log_on = on;
}

int
cons_fb_init(struct cons_fb *fb)
{
	use_fb = fb_init(fb);
	return use_fb;
}

void
kputc(int c)
{
	if (use_fb)
		fb_putc(c, log_on);
	else
		vga_putc(c, log_on);
}
