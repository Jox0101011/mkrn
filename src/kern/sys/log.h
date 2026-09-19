/*
 * log.h - logging padronizado do kernel
 *
 * formato: [ timestamp ] subsistema: mensagem
 * "fac" (facility/subsistema) pode ser NULL quando a mensagem nao
 * pertence a um subsistema especifico.
 */

#ifndef _SYS_LOG_H_
#define _SYS_LOG_H_

void klog(const char *fac, const char *fmt, ...);
void kwarn(const char *fac, const char *fmt, ...);
void kerror(const char *fac, const char *fmt, ...);
void panic(const char *fmt, ...);
#endif /* !_SYS_LOG_H_ */
