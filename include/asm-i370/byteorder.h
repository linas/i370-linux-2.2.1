#ifndef _I370_BYTEORDER_H
#define _I370_BYTEORDER_H

/* The I370 has no special instructions for rotating or swapping bytes
 * and so the defaults in include/linux/byteorder/big_endian.h get used.
 * Of course, maybe you can tune the perofrmance of this ...
 */

#include <asm/types.h>

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#define __BYTEORDER_HAS_U64__
#endif
#include <linux/byteorder/big_endian.h>

#endif /* _I370_BYTEORDER_H */
