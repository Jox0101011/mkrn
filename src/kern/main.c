/*
 * main.c - ponto de entrada em C do kernel
 *
 * chamado por amd64/boot.S com magic/mbi_phys ainda em registradores
 * de 32 bits (estamos em modo protegido, nao em modo longo).
 */


#include "sys/types.h"
#include "amd64/include/machine/cpufunc.h"
#include "amd64/include/machine/idt.h"
#include "amd64/include/machine/multiboot.h"
#include "amd64/include/machine/pic.h"
#include "amd64/include/machine/pit.h"
#include "amd64/include/machine/pmap.h"
#include "amd64/include/machine/segments.h"
#include "amd64/include/machine/tsc.h"
#include "sys/clock.h"
#include "sys/cons.h"
#include "sys/font.h"
#include "sys/kmalloc.h"
#include "sys/libkern.h"
#include "sys/log.h"
#include "sys/panic.h"
#include "sys/pmm.h"
#include "sys/thread.h"
#include "sys/vmm.h"

/*
 * threads de teste do escalonador: so imprimem a letra delas e cedem
 * a vez, sem fim. resultado esperado no log: A B A B A B...
 */
static void
thread_a(void)
{
	for (;;) {
		klog(NULL, "A");
		yield();
	}
}

static void
thread_b(void)
{
	for (;;) {
		klog(NULL, "B");
		yield();
	}
}

/*
 * primeiro programa de usuario (teste): so um "for (;;) {}" - como
 * ainda nao tem loader de elf, e o codigo de maquina escrito a mao
 * mesmo (jmp pra si mesmo = EB FE). o kernel escreve esses bytes
 * direto na pagina de usuario: pode, porque o bit PAGE_USER da pte
 * so restringe ring3, ring0 sempre enxerga tudo (ver amd64/pmap.c).
 */
static const uint8_t user_main_code[] = { 0xeb, 0xfe };	/* 1: jmp 1b */

#define USER_CODE_VA	0x40000000u
#define USER_STACK_VA	0x40001000u	/* 1 pagina; a thread entra com esp no topo dela */

void
kmain(uint32_t magic, uint32_t mbi_phys)
{
	struct mb2_info *mbi;
	struct mb2_tag_meminfo *mi;
	struct mb2_tag_string *str;
	struct mb2_tag_framebuffer *fbtag;
	int font_ok;

	vga_init();

	/* usa a fonte o quanto antes: da pra rodar sem pmm/vmm/heap
	   nem framebuffer de verdade nenhum, ja que o selftest desenha
	   num buffer estatico proprio. o klog do resultado fica pra
	   depois do tsc_calibrate() la embaixo (sem ele o timestamp
	   sai lixo), mas o desenho em si roda aqui, o mais cedo que da */
	font_ok = font_selftest();

	gdt_init();
	idt_init();
	pic_init();
	tsc_calibrate();
	pit_init(HZ);

	klog(NULL, "mkrn 0.1 (amd64/bios)");
	klog("gdt", "%d descritores carregados (cs=0x%x ds=0x%x tss=0x%x)",
	    NGDT, GSEL_KCODE, GSEL_KDATA, GSEL_TSS);
	klog("idt", "%d vetores de excecao instalados (0-%d)", NEXC, NEXC - 1);
	klog("pic", "8259 remapeado (irq0-15 -> vetor %d-%d), tudo mascarado",
	    IRQ_BASE, IRQ_BASE + NIRQ - 1);
	klog("timer", "pit no canal 0 a %d hz (1 tick = %d ms), irq0 desmascarada",
	    HZ, 1000 / HZ);

	if (!font_ok)
		panic("fonte 8x8: selftest falhou (nenhum pixel aceso)");
	klog("font", "fonte 8x8 (cp437) carregada, renderer testado (glifo de exemplo)");

	if (magic != MB2_BOOTLOADER_MAGIC) {
		klog("boot", "magic multiboot invalido: 0x%x", magic);
		goto idle;
	}

	mbi = (struct mb2_info *)(uintptr_t)mbi_phys;

	mi = (struct mb2_tag_meminfo *)mb2_find_tag(mbi, MB2_TAG_BASIC_MEMINFO);
	if (mi != NULL)
		klog("boot", "memoria: %uK lower, %uK upper", mi->mem_lower, mi->mem_upper);

	str = (struct mb2_tag_string *)mb2_find_tag(mbi, MB2_TAG_BOOT_LOADER_NAME);
	if (str != NULL)
		klog("boot", "bootloader: %s", str->string);

	str = (struct mb2_tag_string *)mb2_find_tag(mbi, MB2_TAG_CMDLINE);
	if (str != NULL && str->string[0] != '\0')
		klog("boot", "cmdline: %s", str->string);

	pmm_bootstrap(mbi);
	pmap_init();

	/*
	 * framebuffer linear: so depois do pmap_init() porque precisa
	 * de vmm_map() de verdade (a tabela temporaria do boot.S so
	 * cobre os primeiros 4m, e o endereco do framebuffer costuma
	 * ficar bem mais alto - mmio de placa de video, nao ram). ate
	 * aqui, e daqui pra tras se essa tag nao vier ou o bpp nao for
	 * suportado, o console continua no vga de texto (vga_init() la
	 * em cima) - e o fallback que foi pedido.
	 */
	fbtag = (struct mb2_tag_framebuffer *)mb2_find_tag(mbi, MB2_TAG_FRAMEBUFFER);
	if (fbtag != NULL && fbtag->fb_type == MB2_FB_TYPE_RGB &&
	    fbtag->addr <= 0xffffffffULL) {
		struct cons_fb cfb;
		uint32_t base, len, addr;

		base = (uint32_t)fbtag->addr;
		len = fbtag->pitch * fbtag->height;

		for (addr = base & ~(PAGE_SIZE - 1); addr < base + len; addr += PAGE_SIZE)
			vmm_map(addr, addr, PAGE_PRESENT | PAGE_WRITE);

		cfb.addr = base;
		cfb.pitch = fbtag->pitch;
		cfb.width = fbtag->width;
		cfb.height = fbtag->height;
		cfb.bpp = fbtag->bpp;
		cfb.red_pos = fbtag->red_pos;
		cfb.red_size = fbtag->red_size;
		cfb.green_pos = fbtag->green_pos;
		cfb.green_size = fbtag->green_size;
		cfb.blue_pos = fbtag->blue_pos;
		cfb.blue_size = fbtag->blue_size;

		if (cons_fb_init(&cfb))
			klog("cons", "framebuffer linear %ux%u@%ubpp em 0x%x",
			    cfb.width, cfb.height, cfb.bpp, cfb.addr);
		else
			klog("cons", "framebuffer linear com bpp %u, sem suporte - continua no vga",
			    cfb.bpp);
	} else {
		klog("cons", "sem framebuffer linear rgb do multiboot2 - continua no vga");
	}

	/* teste de fumaca da api de vmm: mapeia uma pagina nova num
	   endereco virtual que nao existe em mapeamento nenhum ainda,
	   escreve, le de volta, desmapeia e confirma que sumiu */
	{
		uint32_t pa = pmm_alloc();
		uint32_t va = 0xd0000000;

		if (pa == PMM_ENOMEM) {
			klog("vmm", "teste pulado: sem pagina livre");
		} else {
			vmm_map(va, pa, PAGE_PRESENT | PAGE_WRITE);
			*(volatile uint32_t *)va = 0xcafef00d;

			klog("vmm", "teste: va=0x%x -> pa=0x%x, escreveu/leu 0x%x",
			    va, vmm_extract(va), *(volatile uint32_t *)va);

			vmm_unmap(va);
			klog("vmm", "teste: depois do unmap, vmm_extract=0x%x",
			    vmm_extract(va));

			pmm_free(pa);
		}
	}

	kheap_init();

	/* teste de fumaca do kmalloc/kfree: aloca uns blocos de
	   tamanhos diferentes, escreve, libera o do meio e confere
	   que a fusao com o vizinho da espaco pra uma alocacao maior */
	{
		char *a = kmalloc(64);
		char *b = kmalloc(128);
		char *c = kmalloc(32);

		klog("kheap", "teste: kmalloc(64)=0x%x kmalloc(128)=0x%x kmalloc(32)=0x%x",
		    (uint32_t)a, (uint32_t)b, (uint32_t)c);

		if (a != NULL)
			a[0] = 'A';
		if (b != NULL)
			b[0] = 'B';
		if (c != NULL)
			c[0] = 'C';

		kfree(b);

		char *d = kmalloc(100);
		klog("kheap", "teste: depois de kfree(b), kmalloc(100)=0x%x", (uint32_t)d);

		kfree(a);
		kfree(c);
		kfree(d);
	}

	klog(NULL, "main: inicializacao concluida");

	sched_init();
	thread_create(&kernel_task, thread_a);
	thread_create(&kernel_task, thread_b);
	klog("sched", "threads a e b criadas");

	/*
	 * primeira task de usuario (teste): uma pagina de codigo (o
	 * loop acima) e uma de stack, as duas com PAGE_USER - e so
	 * isso que diferencia elas de qualquer pagina de kernel, e
	 * nenhuma outra pagina do sistema tem esse bit. um acesso de
	 * ring3 fora dessas duas paginas da #pf na hora (protecao=1,
	 * modo=usuario no log de amd64/trap.c:pagefault_handler()) -
	 * e assim que "usuario nao acessa memoria de kernel" e
	 * garantido aqui, sem precisar de pgdir separado ainda.
	 */
	{
		uint32_t code_pa = pmm_alloc();
		uint32_t stack_pa = pmm_alloc();

		if (code_pa == PMM_ENOMEM || stack_pa == PMM_ENOMEM) {
			klog("user", "sem pagina livre pra primeira task de usuario, pulando");
		} else {
			struct task *utask;

			vmm_map(USER_CODE_VA, code_pa, PAGE_PRESENT | PAGE_USER);
			memcpy((void *)USER_CODE_VA, user_main_code, sizeof(user_main_code));

			vmm_map(USER_STACK_VA, stack_pa, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

			utask = task_create();
			thread_create_user(utask, USER_CODE_VA, USER_STACK_VA + PAGE_SIZE);

			klog("user", "task de usuario criada: eip=0x%x esp=0x%x",
			    USER_CODE_VA, USER_STACK_VA + PAGE_SIZE);
		}
	}

	sti();		/* so agora comeca a receber a irq0 do timer */

	scheduler_start();	/* nunca retorna - dali em diante e a e b se revezando */

idle:
	for (;;)
		__asm__ volatile("hlt");
}
