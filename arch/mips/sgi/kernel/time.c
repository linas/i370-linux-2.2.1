/* $Id: time.c,v 1.1 1999/02/08 06:21:24 linas Exp $
 * time.c: Generic SGI time_init() code, this will dispatch to the
 *         appropriate per-architecture time/counter init code.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <linux/init.h>

extern void indy_timer_init(void);

__initfunc(void time_init(void))
{
	/* XXX assume INDY for now XXX */
	indy_timer_init();
}
