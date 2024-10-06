/*
 * include/asm-i370/cache.h
 */
#ifndef __ARCH_I370_CACHE_H
#define __ARCH_I370_CACHE_H

#include <linux/config.h>

/* XXX these values are used somehow by the mem management code
 * to align stuff onto cache lines and thus improve performance
 * e.g. for SMP to avoid cache-line sharing. But the actual impact
 * is not clear to me. Also, these figures should be adjusted for 
 * the actual hardware model; different models will have different
 * configs.
 */

/* bytes per L1 cache line */
#define        L1_CACHE_BYTES  16      
#define        L1_CACHE_ALIGN(x)       (((x)+(L1_CACHE_BYTES-1))&~(L1_CACHE_BYTES-1))

#endif
