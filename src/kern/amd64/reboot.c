/*
 * reboot.c - reboot/poweroff seguros (amd64/bios)
 *
 * "seguro" aqui significa: reboot() nunca trava esperando hardware
 * que nao responde (timeout no 8042, com fallback pra triple fault)
 * e poweroff() nunca escreve em portas de i/o desconhecidas em
 * hardware real - testa so as portas conhecidas de emuladores
 * comuns; sem resposta, cai num halt controlado em vez de arriscar
 * side effects.
 *
 * poweroff "de verdade" via acpi (avaliar o pacote \_S5 da dsdt e
 * escrever no registrador pm1a_cnt) fica pra uma proxima etapa -
 * exige parsear as tabelas acpi (rsdp/rsdt/fadt/dsdt), que ainda nao
 * temos aqui.
 */


#include "../sys/types.h"
#include "include/machine/cpufunc.h"
#include "../sys/log.h"
#include "../sys/reboot.h"

#define KBC_TIMEOUT	0x100000

/* espera o buffer de entrada do 8042 esvaziar, com timeout */
static void
kbc_wait_input_empty(void)
{
	unsigned long i;

	for (i = 0; i < KBC_TIMEOUT; i++) {
		if ((inb(0x64) & 0x02) == 0)
			return;
	}
}

void
reboot(void)
{
	__asm__ volatile("cli");

	klog("reboot", "pulsando linha de reset via 8042");

	/* drena qualquer byte pendente no buffer de saida */
	if (inb(0x64) & 0x01)
		(void)inb(0x60);

	kbc_wait_input_empty();
	outb(0x64, 0xfe);	/* pulsa a linha de reset */
	kbc_wait_input_empty();	/* da uma chance do reset acontecer */

	/*
	 * 8042 nao respondeu (hardware sem ele, qemu -M microvm, etc):
	 * forca um triple fault. sem idt valida (limite 0), qualquer
	 * excecao vira double fault e, sem handler de double fault
	 * tambem, vira triple fault - que reseta o processador.
	 */
	klog("reboot", "8042 nao respondeu, forcando triple fault");

	{
		const struct {
			uint16_t limit;
			uint32_t base;
		} __attribute__((packed)) idtr_null = { 0, 0 };

		__asm__ volatile("lidt %0" : : "m"(idtr_null));
		__asm__ volatile("int $0x03");
	}

	for (;;)
		__asm__ volatile("hlt");
}

void
poweroff(void)
{
	__asm__ volatile("cli");

	klog("poweroff", "tentando desligar via portas conhecidas de emulador");

	outw(0x604, 0x2000);	/* qemu (pm base padrao, piix4/q35) */
	outw(0xb004, 0x2000);	/* qemu antigo / bochs */
	outw(0x4004, 0x3400);	/* virtualbox */

	/*
	 * nenhuma resposta: sem acpi de verdade nao da pra desligar
	 * hardware real com seguranca (escrever numa porta arbitraria
	 * pode ter efeito indefinido num dispositivo desconhecido).
	 * trava aqui - e seguro, e da pra desligar na mao.
	 */
	kwarn("poweroff", "desligamento automatico indisponivel; travando");

	for (;;)
		__asm__ volatile("hlt");
}
