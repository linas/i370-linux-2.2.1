/* $Id: mp.c,v 1.1.1.1 1999/02/08 06:21:19 linas Exp $
 * mp.c:  OpenBoot Prom Multiprocessor support routines.  Don't call
 *        these on a UP or else you will halt and catch fire. ;)
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>

#include <asm/openprom.h>
#include <asm/oplib.h>

extern void restore_current(void);

/* Start cpu with prom-tree node 'cpunode' using context described
 * by 'ctable_reg' in context 'ctx' at program counter 'pc'.
 *
 * XXX Have to look into what the return values mean. XXX
 */
int
prom_startcpu(int cpunode, struct linux_prom_registers *ctable_reg, int ctx, char *pc)
{
	int ret;
	unsigned long flags;

	save_flags(flags); cli();
	switch(prom_vers) {
	case PROM_V0:
	case PROM_V2:
	case PROM_AP1000:
	default:
		ret = -1;
		break;
	case PROM_V3:
		ret = (*(romvec->v3_cpustart))(cpunode, (int) ctable_reg, ctx, pc);
		break;
	};
	restore_current();
	restore_flags(flags);

	return ret;
}

/* Stop CPU with device prom-tree node 'cpunode'.
 * XXX Again, what does the return value really mean? XXX
 */
int
prom_stopcpu(int cpunode)
{
	int ret;
	unsigned long flags;

	save_flags(flags); cli();
	switch(prom_vers) {
	case PROM_V0:
	case PROM_V2:
	case PROM_AP1000:
	default:
		ret = -1;
		break;
	case PROM_V3:
		ret = (*(romvec->v3_cpustop))(cpunode);
		break;
	};
	restore_current();
	restore_flags(flags);

	return ret;
}

/* Make CPU with device prom-tree node 'cpunode' idle.
 * XXX Return value, anyone? XXX
 */
int
prom_idlecpu(int cpunode)
{
	int ret;
	unsigned long flags;

	save_flags(flags); cli();
	switch(prom_vers) {
	case PROM_V0:
	case PROM_V2:
	case PROM_AP1000:
	default:
		ret = -1;
		break;
	case PROM_V3:
		ret = (*(romvec->v3_cpuidle))(cpunode);
		break;
	};
	restore_current();
	restore_flags(flags);

	return ret;
}

/* Resume the execution of CPU with nodeid 'cpunode'.
 * XXX Come on, somebody has to know... XXX
 */
int
prom_restartcpu(int cpunode)
{
	int ret;
	unsigned long flags;

	save_flags(flags); cli();
	switch(prom_vers) {
	case PROM_V0:
	case PROM_V2:
	case PROM_AP1000:
	default:
		ret = -1;
		break;
	case PROM_V3:
		ret = (*(romvec->v3_cpuresume))(cpunode);
		break;
	};
	restore_current();
	restore_flags(flags);

	return ret;
}
