/*
 */

#include <linux/init.h>
#include <linux/kernel.h>
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

int get_irq_list(char *buf)
{
        int len = 0, j;
        len += sprintf(buf+len, "No irqs XXX not implemented\n");
	return len;
}

