/*
 */

#include <linux/init.h>
#include <linux/tasks.h>

unsigned int local_bh_count[NR_CPUS];
unsigned int local_irq_count[NR_CPUS];


__initfunc(void init_IRQ(void))
{
   printk ("init irq\n");
}

void
check_bugs(void)
{
}

