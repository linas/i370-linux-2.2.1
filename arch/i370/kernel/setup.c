
/*
 * arch/i370/kernel/setup.c
 *
 * perform some very very early setup for initialization
 */
#include <linux/config.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/blk.h>

#include <asm/asm.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/trace.h>

// cmd_line is array of 512 in head.S
extern char cmd_line[512];
char saved_command_line[512];


extern void i370_find_devices (unsigned long *memory_start_p, 
				unsigned long *memory_end_p);

/* ========================================================== */
/*
 * set up system trace tables
 */
trc_page_t	*trace_base;

void	setup_trace(unsigned long *memory_start) 
{
	long	i;
	unsigned long cpuid = _stap() | (1<<31);
	unsigned long cr12;

	trc_page_t	*trc;

	/* align on page boundry ... */
	trace_base = (trc_page_t *)(((int)(*memory_start) + 0xfff));
	trace_base = (trc_page_t *)(((int)(trace_base) & 0x7ffff000));

	/* build ciruclar list */
	trc = trace_base;
	for (i=0; i<MAX_TRACE_PAGES; i++) 
	{
		trc->trc_cpuad = cpuid;
		trc->trc_next = (trc+1);
		trc ++;
	}
	trc--;                                  /* back up */
	trc->trc_next = trace_base;	        /* make last trace point to first */

	*memory_start = (long) (trc+1);         /* set new memory start */

	/* put trace table base into control reg 12 */
	cr12 = (unsigned long)trace_base; 
	cr12 |= 0x1;
	_lctl_r12 (cr12);
}

/* ========================================================== */

__initfunc(void setup_arch(char **cmdline_p,
	unsigned long * memory_start_p, unsigned long * memory_end_p))
{
	unsigned long ksp;
	extern int panic_timeout;
	extern char _etext[], _edata[], _end[];

	/* reboot on panic */	
	panic_timeout = 180;
	
	init_task.mm->start_code = PAGE_OFFSET;
	init_task.mm->end_code = (unsigned long) _etext;
	init_task.mm->end_data = (unsigned long) _edata;
//  	init_task.mm->brk = (unsigned long) _end;
  	init_task.mm->brk = (unsigned long) -1;

	/* Save unparsed command line copy for /proc/cmdline */
	strcpy(saved_command_line, cmd_line);
	*cmdline_p = cmd_line;

	/* hardcode the memory size */
 	*memory_start_p = _end;
	*memory_end_p = 0x2000000;  // 32M

	i370_find_devices(memory_start_p,memory_end_p);

	/* hard-code an initrd into the system */
	initrd_start  =  0x200000;  // 2M
	initrd_end    =  0x400000;  // 4M

	/* init_task ksp hasn't been set & its bogus; set it */
	ksp = init_stack;
	ksp = ((ksp +7) >>3 ) << 3;
	init_task.tss.ksp = ksp ;

	setup_trace(memory_start_p);

	/*
	 *	Hack Hack. Initialize the page_hash_table to zeros
	 */
	
	memset(&page_hash_table,0x0,(PAGE_HASH_SIZE * sizeof(void *)));


}

void machine_restart(char *cmd)
{
	printk("machine restart\n");
	i370_halt();
}

void machine_halt(void)
{
   	printk("machine halt\n");
	i370_halt();
}
void machine_power_off(void)
{
	printk("machine power off\n");
	i370_halt();
}

int get_cpuinfo(char *buffer)
{
	unsigned long len = 0;

	len += sprintf(len+buffer,"XXX cpuinfo not implemented");
	return len;
}

/* ============================= END OF FILE ======================== */

