
#include <linux/config.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/delay.h>

#include <asm/pgtable.h>
#include <asm/processor.h>

int _machine = 0;

__initfunc(void setup_arch(char **cmdline_p,
	unsigned long * memory_start_p, unsigned long * memory_end_p))
{
	extern int panic_timeout;
	extern char _etext[], _edata[];
	extern char *klimit;
//	extern unsigned long find_available_memory(void);
// 	extern unsigned long *end_of_DRAM;

	/* reboot on panic */	
	panic_timeout = 180;
	
	init_task.mm->start_code = PAGE_OFFSET;
	init_task.mm->end_code = (unsigned long) _etext;
	init_task.mm->end_data = (unsigned long) _edata;
// 	init_task.mm->brk = (unsigned long) klimit;	

	/* Save unparsed command line copy for /proc/cmdline */
	// strcpy(saved_command_line, cmd_line);
	// *cmdline_p = cmd_line;

// 	*memory_start_p = find_available_memory();
//	*memory_end_p = (unsigned long) end_of_DRAM;

	switch (_machine) {
	default:
		printk("Unknown machine %d in setup_arch()\n", _machine);
	}
}
