
#include <linux/config.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/blk.h>

#include <asm/pgtable.h>
#include <asm/processor.h>

// cmd_line is array of 512 in head.S
extern char cmd_line[512];
char saved_command_line[512];

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
	*memory_end_p = 0x4000000;  // 64M

	/* hard-code an initrd into the system */
	initrd_start =   0x800000;    // 8M
	initrd_end  =   0x1000000;    // 16M

	/* init_task ksp hasn't been set & its bogus; set it */
	ksp = init_stack;
	ksp = ((ksp +7) >>3 ) << 3;
	init_task.tss.ksp = ksp ;
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


