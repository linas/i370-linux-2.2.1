/*
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/tasks.h>

unsigned int local_bh_count[NR_CPUS];
unsigned int local_irq_count[NR_CPUS];


__initfunc(void init_IRQ(void))
{
	int i;
	printk ("init irq\n");
	for (i=0; i<NR_CPUS; i++) {
		local_bh_count[i] = 0;
		local_irq_count[i] = 0;
	}
}

int get_irq_list(char *buf)
{
        int len = 0;
        len += sprintf(buf+len, "No irqs XXX not implemented\n");
	return len;
}

