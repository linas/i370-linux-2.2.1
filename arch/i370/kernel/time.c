/*
 */

#include <linux/init.h>


/* keep track of when we need to update the rtc */
unsigned long last_rtc_update = 0;


__initfunc(void time_init(void))
{
   printk ("time init\n");
}

/*
 * This version of gettimeofday has microsecond resolution.
 */
void do_gettimeofday(struct timeval *tv)
{
#if 0
        unsigned long flags;

        save_flags(flags);
        cli();
        *tv = xtime;
        /* XXX we don't seem to have the decrementers synced properly yet
         * */
#ifndef __SMP__
        tv->tv_usec += (decrementer_count - get_dec())
            * count_period_num / count_period_den;
        if (tv->tv_usec >= 1000000) {
                tv->tv_usec -= 1000000;
                tv->tv_sec++;
        }
#endif
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


