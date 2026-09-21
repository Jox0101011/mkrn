/*
 * kernbase.h - kernbase/kernphys, e so
 *
 * deliberadamente sem incluir nada (nem types.h): isso e incluido
 * tanto de c/asm (pmap.h, boot.S) quanto do linker script
 * (amd64/kern.lds, via cpp) - se tivesse typedef ou qualquer coisa
 * alem de #define aqui, o preprocessador deixaria esse texto no meio
 * do linker script e o ld nao ia saber o que fazer com aquilo.
 */

#ifndef _MACHINE_KERNBASE_H_
#define _MACHINE_KERNBASE_H_

/* onde o kernel "de verdade" (nao o trampolim de boot.S) roda */
#define KERNBASE	0xC0000000

/* onde o grub carrega o kernel fisicamente */
#define KERNPHYS	0x00100000

#endif /* !_MACHINE_KERNBASE_H_ */
