
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>

#include <asm/asm.h>


/* keep track of when we need to update the rtc */
unsigned long last_rtc_update = 0;


__initfunc(void time_init(void))
{
	cr0_t cr0;

	unsigned long skippy;
	unsigned long long tod;
	printk ("enter time init\n");

	/*------------------------------------------------------------*/
	/* Enable clock comparator interrupts to present themselves   */
	/*------------------------------------------------------------*/
	cr0.raw = _stctl_r0();
	cr0.bits.clksm |= 1;
	_lctl_r0(cr0.raw);

	/* enable interrupts */
	__sti();

	/* hack alert XXX I suppose that we should check that the
	 * system TOD clock is not in the stopped state, and if it is, 
	 * set it running again... 
	 */
	/* grab the system time of day clock, add 200 milliseconds,
	 * and store that in the clock comparator.  That should start 
	 * the interupts going.  We pick a large value just in case
	 * VM decides to play games with us.
	 */
	tod = _stck();
	tod += 20 * (1000000/HZ) << 12;
	_sckc (tod);

	/* wait for an interrupt */
	skippy = jiffies;
	if (! _i370_hercules_guest_p())
		while (skippy == jiffies) {/* empty */};

	printk ("exit time init\n");
}

/*
 * This version of gettimeofday has microsecond resolution.
 */
void do_gettimeofday(struct timeval *tv)
{
/* XXX unhack this ; this is copied from ppc .... */
#if 0
        unsigned long flags;

        save_flags(flags);
        cli();
        *tv = xtime;
        tv->tv_usec += (decrementer_count - get_dec())
            * count_period_num / count_period_den;
        if (tv->tv_usec >= 1000000) {
                tv->tv_usec -= 1000000;
                tv->tv_sec++;
        }
        restore_flags(flags);
#endif
}

void do_settimeofday(struct timeval *tv)
{
#if 0
        unsigned long flags;
        int frac_tick;

        last_rtc_update = 0; /* so the rtc gets updated soon */

        frac_tick = tv->tv_usec % (1000000 / HZ);
        save_flags(flags);
        cli();
        xtime.tv_sec = tv->tv_sec;
        xtime.tv_usec = tv->tv_usec - frac_tick;
        set_dec(frac_tick * count_period_den / count_period_num);
        time_adjust = 0;                /* stop active adjtime() */
        time_status |= STA_UNSYNC;
        time_state = TIME_ERROR;        /* p. 24, (a) */
        time_maxerror = NTP_PHASE_LIMIT;
        time_esterror = NTP_PHASE_LIMIT;
        restore_flags(flags);
#endif
}


