/*
 * reboot.h - reboot/poweroff do kernel
 *
 * assim como clock.h/md_uptime(), a assinatura e machine-independent
 * mas a implementacao (amd64/reboot.c) e especifica de arquitetura.
 */

#ifndef _SYS_REBOOT_H_
#define _SYS_REBOOT_H_

void reboot(void) __attribute__((noreturn));
void poweroff(void) __attribute__((noreturn));

#endif /* !_SYS_REBOOT_H_ */
