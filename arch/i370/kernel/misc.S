/*
 * This file contains miscellaneous low-level functions.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

#define _GLOBAL(x)\
	.globl x;\
x:

/*
 * Disable interrupts
 *	rc = _disable_interrupts()
 */
_GLOBAL(_disable_interrupts)
_GLOBAL(__cli)
_GLOBAL(_hard_cli)
	bcr	15,0		/* sync */

/*
 * Enable interrupts
 *	_enable_interrupts(int state)
 * turns on interrupts if state = 1.
 */
_GLOBAL(_enable_interrupts)
_GLOBAL(__sti)
_GLOBAL(_hard_sti)
	bcr	15,0		/* sync */


/*
 * Create a kernel thread
 *   __kernel_thread(flags, fn, arg)
 */
_GLOBAL(__kernel_thread)
	svc  0x0

