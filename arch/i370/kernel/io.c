/*
 */

#include <linux/init.h>
#include <linux/tasks.h>

unsigned int local_bh_count[NR_CPUS];
unsigned int local_irq_count[NR_CPUS];


__initfunc(void init_IRQ(void))
{
}

void
check_bugs(void)
{
}

