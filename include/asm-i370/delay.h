#ifndef _I370_DELAY_H
#define _I370_DELAY_H

/*
 * copied from linux/include/asm-ppc/delay.h
 * copyrights from the above file may apply to this file.
 * Copyright (c) 1999 Linas Vepstas linas@linas.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

extern unsigned long loops_per_sec;

extern __inline__ void __delay(unsigned int loops)
{
	while (loops) loops --;
}

extern __inline__ void udelay(unsigned long usecs)
{
#ifdef MULDI3_FIXED
        unsigned long long loops;

	/* stunt to avoid a divide */
	/* compute (usecs * 2^32 / 10^6) * loops_per_sec / 2^32 */
	loops = usecs;
	loops *= 0x10c6;		/* 2^32 / 10^6 */

	loops *= loops_per_sec;
        loops = loops >> 32;
#else /* MULDI3_FIXED */
	unsigned long loops;
	loops = loops_per_sec / 1000000;
	loops *= usecs;
#endif /* MULDI3_FIXED */

	__delay(loops);
}

#endif /* defined(_I370_DELAY_H) */
