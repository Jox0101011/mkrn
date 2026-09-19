/*
 * param.h - macros genericas do kernel
 */

#ifndef _SYS_PARAM_H_
#define _SYS_PARAM_H_

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

#define roundup(x, y)	((((x) + ((y) - 1)) / (y)) * (y))
#define rounddown(x, y)	(((x) / (y)) * (y))

#define nitems(x)	(sizeof(x) / sizeof((x)[0]))

#endif /* !_SYS_PARAM_H_ */
