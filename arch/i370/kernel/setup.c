
#include <linux/config.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/delay.h>

#include <asm/pgtable.h>
#include <asm/processor.h>

// cmd_line is array of 512 in head.S
extern char *cmd_line;
char saved_command_line[512];

__initfunc(void setup_arch(char **cmdline_p,
	unsigned long * memory_start_p, unsigned long * memory_end_p))
{
	extern int panic_timeout;
	extern char _etext[], _edata[], _end[];
//	extern unsigned long find_available_memory(void);
// 	extern unsigned long *end_of_DRAM;

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

 	*memory_start_p = _end;
//	*memory_end_p = (unsigned long) end_of_DRAM;
	*memory_end_p = 0x2000000;  // 32M

}

void machine_restart(char *cmd)
{
   printk("machine restart\n");
}

void machine_halt(void)
{
   printk("machine halt\n");
}
void machine_power_off(void)
{
   printk("machine pwer off\n");
}

int get_cpuinfo(char *buffer)
{
	unsigned long len = 0;

	len += sprintf(len+buffer,"XXX cpuinfo not implemented");
	return len;
}


