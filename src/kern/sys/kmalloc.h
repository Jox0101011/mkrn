/*
 * kmalloc.h - heap do kernel
 *
 *   kmalloc()/kfree()
 *        |
 *        v
 *   heap do kernel (lista encadeada de blocos, ver subr_kmalloc.c)
 *        |
 *        v
 *   vmm (vmm_map)
 *        |
 *        v
 *   pmm (pmm_alloc)
 *        |
 *        v
 *   ram
 */

#ifndef _SYS_KMALLOC_H_
#define _SYS_KMALLOC_H_

#include <stddef.h>

void kheap_init(void);

void *kmalloc(size_t size);
void kfree(void *ptr);

#endif /* !_SYS_KMALLOC_H_ */
