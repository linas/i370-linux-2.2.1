/*
 */

#include <linux/init.h>


/* keep track of when we need to update the rtc */
unsigned long last_rtc_update = 0;


__initfunc(void time_init(void))
{
}

