/*
 * i370 architecture timex specifications
 */
#ifndef _I370_TIMEX_H
#define _I370_TIMEX_H

/* XXX this is all wrong */
#define CLOCK_TICK_RATE	1193180 /* Underlying HZ */
#define CLOCK_TICK_FACTOR	20	/* Factor of both 1000000 and CLOCK_TICK_RATE */
#define FINETUNE ((((((long)LATCH * HZ - CLOCK_TICK_RATE) << SHIFT_HZ) * \
	(1000000/CLOCK_TICK_FACTOR) / (CLOCK_TICK_RATE/CLOCK_TICK_FACTOR)) \
		<< (SHIFT_SCALE-SHIFT_HZ)) / HZ)

typedef unsigned long cycles_t;

/*
 * For the "cycle" counter we use the timebase lower half.
 * Currently only used on SMP.
 *
 */

extern cycles_t cacheflush_time;

static inline cycles_t get_cycles(void)
{
#ifdef __SMP__
	cycles_t ret;

	__asm__("mftb %0" : "=r" (ret) : );
	return ret;
#else
	return 0;
#endif
}

#endif
