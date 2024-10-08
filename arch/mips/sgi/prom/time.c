/*
 * time.c: Extracting time information from ARCS prom.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 *
 * $Id: time.c,v 1.1.1.1 1999/02/08 06:21:24 linas Exp $
 */
#include <linux/init.h>
#include <asm/sgialib.h>

__initfunc(struct linux_tinfo *prom_gettinfo(void))
{
	return romvec->get_tinfo();
}

__initfunc(unsigned long prom_getrtime(void))
{
	return romvec->get_rtime();
}
