/* 
 * Copyright (C) 1996, 1997 Claus-Justus Heine.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *
 * $Source: /home2/linas/archive/backlot/cvs-2014-11-21/cvsroot/linux/drivers/char/ftape/lowlevel/ftape-setup.c,v $
 * $Revision: 1.1.1.1 $
 * $Date: 1999/02/08 06:20:06 $
 *
 *      This file contains the code for processing the kernel command
 *      line options for the QIC-40/80/3010/3020 floppy-tape driver
 *      "ftape" for Linux.
 */

#include <linux/config.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <asm/segment.h>

#include <linux/ftape.h>
#if LINUX_VERSION_CODE >= KERNEL_VER(2,1,16)
#include <linux/init.h>
#else
#define __initdata
#define __initfunc(__arg) __arg
#endif
#include "../lowlevel/ftape-tracing.h"
#include "../lowlevel/fdc-io.h"

static struct param_table {
	const char *name;
	int *var;
	int def_param;
	int min;
	int max;
} config_params[] __initdata = {
#ifndef CONFIG_FT_NO_TRACE_AT_ALL
	{ "tracing",   &ftape_tracing,     3,              ft_t_bug, ft_t_any},
#endif
	{ "ioport",    &ft_fdc_base,       CONFIG_FT_FDC_BASE,     0x0, 0xfff},
	{ "irq",       &ft_fdc_irq,        CONFIG_FT_FDC_IRQ,        2,    15},
	{ "dma",       &ft_fdc_dma,        CONFIG_FT_FDC_DMA,        0,     3},
	{ "threshold", &ft_fdc_threshold,  CONFIG_FT_FDC_THR,         1,    16},
	{ "datarate",  &ft_fdc_rate_limit, CONFIG_FT_FDC_MAX_RATE, 500,  2000},
	{ "fc10",      &ft_probe_fc10,     CONFIG_FT_PROBE_FC10,     0,     1},
	{ "mach2",     &ft_mach2,          CONFIG_FT_MACH2,          0,     1}
};

__initfunc(void ftape_setup(char *str, int *ints))
{
	int i;
	int param;
	TRACE_FUN(ft_t_flow);

	if (str) {
		for (i=0; i < NR_ITEMS(config_params); i++) {
			if (strcmp(str,config_params[i].name) == 0){
				if (ints[0]) {
					param = ints[1];
				} else {
					param = config_params[i].def_param;
				}
				if (param < config_params[i].min ||
				    param > config_params[i].max) {
					TRACE(ft_t_err,
					"parameter %s out of range %d ... %d",
					      config_params[i].name,
					      config_params[i].min,
					      config_params[i].max);
					TRACE_EXIT;
				}
				if(config_params[i].var) {
					TRACE(ft_t_info, "%s=%d", str, param);
					*config_params[i].var = param;
				}
				TRACE_EXIT;
			}
		}
	}
	if (str) {
		TRACE(ft_t_err, "unknown ftape option [%s]", str);
		
		TRACE(ft_t_err, "allowed options are:");
		for (i=0; i < NR_ITEMS(config_params); i++) {
			TRACE(ft_t_err, " %s",config_params[i].name);
		}
	} else {
		TRACE(ft_t_err, "botched ftape option");
	}
	TRACE_EXIT;
}
