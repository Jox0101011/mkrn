/*
 * pic.c - 8259a (pic), remapeado pra vetores 32-47
 *
 * por padrao o bios deixa irq0-7 nos vetores 8-15 e irq8-15 em
 * 0x70-0x77 - a primeira faixa cai em cima das excecoes da cpu
 * (0-31), entao precisa remapear antes de desmascarar qualquer irq.
 *
 * tudo comeca mascarado depois do init; cada driver desmascara a
 * propria irq quando estiver pronto pra receber (irq_install() nao
 * desmascara sozinho - fica explicito).
 */


#include "../sys/types.h"
#include "include/machine/cpufunc.h"
#include "include/machine/pic.h"

#define ICW1_INIT	0x10
#define ICW1_ICW4	0x01
#define ICW4_8086	0x01

#define PIC_EOI		0x20
#define PIC_READ_ISR	0x0b	/* ocw3: proxima leitura no cmd retorna o isr */

static void
io_wait(void)
{
	/* porta 0x80 nao e usada por nada num pc; um outb nela da um
	   atraso curto o suficiente pro pic processar o comando anterior */
	outb(0x80, 0);
}

void
pic_init(void)
{
	/* icw1: inicia a sequencia, avisa que vem icw4 */
	outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
	io_wait();

	/* icw2: vetor base de cada pic */
	outb(PIC1_DATA, IRQ_BASE);		/* irq0-7  -> 32-39 */
	io_wait();
	outb(PIC2_DATA, IRQ_BASE + 8);		/* irq8-15 -> 40-47 */
	io_wait();

	/* icw3: cascata - mestre tem a escrava pendurada na irq2 (bit2);
	   escrava se identifica pela irq2 (id 2) */
	outb(PIC1_DATA, 0x04);
	io_wait();
	outb(PIC2_DATA, 0x02);
	io_wait();

	/* icw4: modo 8086 */
	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	/* mascara tudo */
	outb(PIC1_DATA, 0xff);
	outb(PIC2_DATA, 0xff);
}

void
pic_mask(unsigned irq)
{
	uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
	uint8_t bit = 1 << (irq & 7);

	outb(port, inb(port) | bit);
}

void
pic_unmask(unsigned irq)
{
	uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
	uint8_t bit = 1 << (irq & 7);

	outb(port, inb(port) & ~bit);

	/* irq8-15 chegam na cpu pela cascata na irq2 do mestre - sem
	   isso desmascarado tambem, nada da escrava chega, mesmo que a
	   propria escrava ja tenha desmascarado a irq pedida */
	if (irq >= 8)
		outb(PIC1_DATA, inb(PIC1_DATA) & (uint8_t)~(1 << 2));
}

/* in-service register (ocw3): bit n ligado = irq n sendo atendida agora */
static uint16_t
pic_read_isr(void)
{
	outb(PIC1_CMD, PIC_READ_ISR);
	outb(PIC2_CMD, PIC_READ_ISR);
	return ((uint16_t)inb(PIC2_CMD) << 8) | inb(PIC1_CMD);
}

/*
 * true se a irq 7/15 for espuria: a cpu foi acordada mas o isr nao
 * confirma que a irq esta mesmo em atendimento (ruido eletrico na
 * linha, tipico so dessas duas em hardware real). quem chama isso
 * tem que checar ANTES de tratar a irq como se fosse de verdade -
 * nao faz sentido rodar o handler registrado pra um evento que na
 * pratica nao aconteceu.
 */
int
pic_is_spurious(unsigned irq)
{
	if (irq == 7 && !(pic_read_isr() & 0x0080))
		return 1;

	if (irq == 15 && !(pic_read_isr() & 0x8000)) {
		/* a escrava sinalizou a cascata mesmo sendo espuria nela -
		   o mestre ainda espera o eoi dessa cascata */
		outb(PIC1_CMD, PIC_EOI);
		return 1;
	}

	return 0;
}

void
pic_eoi(unsigned irq)
{
	if (irq >= 8)
		outb(PIC2_CMD, PIC_EOI);
	outb(PIC1_CMD, PIC_EOI);
}
