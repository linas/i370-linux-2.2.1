
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
#include <asm/vmdiag.h>
#include <asm/psa.h>

// cmd_line is array of 512 in head.S
extern char cmd_line[512];
 
// CPUID is the result of a STIDP
unsigned char CPUID[8];
 
// CPU Details
CPU_t cpu_details[NR_CPUS];

char saved_command_line[512];


extern void i370_find_devices (unsigned long *memory_start_p, 
				unsigned long *memory_end_p);
struct scatter_block {
  struct scatter_block * next;
  long                 len;
  long                 offset;
  char                 data[0];
};

void scatter_move(void * area, struct scatter_block * first_block)
{
  struct scatter_block *  block;
  for (block = first_block; block; block = block->next){
    if (block->len > 0)
      memcpy((char *) area + block->offset, block->data, block->len);
    else 
      memset((char *) area + block->offset, 0, -(block->len));
  }
}
  
void setup_psa(void)
{
  _PSA_.initflag.hercules |= _i370_hercules_guest_p();
  
  _PSA_.Current = &init_task;		/* static, is in fact in the constant data */
  
  _PSA_.cpuadp = _stap();
  _PSA_.cpuadl = _stap();
  _PSA_.cpuno = 1;
  _PSA_.cpumask = 0x0001;
  
 /* that is, really, very minimal, in an smp env. it will fail blatantly */

} 

/* ========================================================== */
/*
 * set up system trace tables
 */
trc_page_t	*trace_base;

void	
setup_trace(unsigned long *memory_start) 
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
	unsigned long mycpu, i_cpu;
	extern int panic_timeout;
	extern char _etext[], _edata[], _end[];


	setup_psa();		/*  */

	/* reboot on panic */	
	panic_timeout = 180;
	
	/* query & construct CPU info */
	__asm__ __volatile__ ("
		STIDP	%0"
		: "=m" (CPUID) );
 
	memset(cpu_details, 0, sizeof(cpu_details));
	mycpu = _stap();
	cpu_details[0].CPU_address = mycpu;
	sprintf(cpu_details[0].CPU_name,"S390%03d",mycpu);
	cpu_details[0].CPU_status  = 0;
	for (i_cpu = 1; i_cpu < NR_CPUS; i_cpu++) {
		if (i_cpu != mycpu) {
			if (_sigp(i_cpu, SIGPSENS) != 3) {
				cpu_details[i_cpu].CPU_address = i_cpu;
				sprintf(cpu_details[i_cpu].CPU_name,"S390%03d",i_cpu);
				cpu_details[i_cpu].CPU_status = CPU_stopped;
			}
			else
			{
				cpu_details[i_cpu].CPU_status = CPU_inoprtv;
			}
		}
	}

	/* set up memory layout */
	init_task.mm->start_code = PAGE_OFFSET;
	init_task.mm->end_code = (unsigned long) _etext;
	init_task.mm->end_data = (unsigned long) _edata;
//  	init_task.mm->brk = (unsigned long) _end;
  	init_task.mm->brk = (unsigned long) -1;

	/* Save unparsed command line copy for /proc/cmdline */
	strcpy(saved_command_line, cmd_line);
	*cmdline_p = cmd_line;

	*memory_start_p = (unsigned long) _end;
	/* If we're running under VM the first byte of the CPUID will be 0xff
	 * in which case we can DIAGNOSE the amount of available memory */
	if (CPUID[0] == 0xff) {
	        *memory_end_p = VM_Diagnose_Code_60();
	} else {
		/* hardcode the memory size */
		*memory_end_p = 0x2000000;  // 32M
	}

	i370_find_devices(memory_start_p,memory_end_p);

	/* hard-code an initrd into the system */
	initrd_start  =  0x200000;  // 2M
	initrd_end    =  0x500000;  // 5M

	/* init_task ksp hasn't been set & its bogus; set it */
	ksp = init_stack;
	init_task.tss.ksp = ksp ;
	init_task.tss.kstp = ksp + MIN_STACK_SIZE;

	setup_trace(memory_start_p);

	/*
	 *	Hack Hack. Initialize the page_hash_table to zeros
	 */
	
	memset(&page_hash_table,0x0,(PAGE_HASH_SIZE * sizeof(void *)));


}

void 
machine_restart(char *cmd)
{
	printk("machine restart XXX not implemented\n");
	i370_halt();
}

void 
machine_halt(void)
{
   	printk("machine halt\n");
	i370_halt();
}

void 
machine_power_off(void)
{
	printk("machine power off\n");
	i370_halt();
}

int 
get_cpuinfo(char *buffer)
{
	unsigned long len = 0, i_cpu;

	len += sprintf(len+buffer,"Processor: %08X\n",CPUID);
	for (i_cpu = 0; i_cpu < NR_CPUS; i_cpu++) {
		if (cpu_details[i_cpu].CPU_status != CPU_inoprtv) {
			len += sprintf(len+buffer,"CPU: %s Address: %04d Status: %08X\n",
				cpu_details[i_cpu].CPU_name,
				cpu_details[i_cpu].CPU_address,
				cpu_details[i_cpu].CPU_status);
		}
	}

	return len;
}

/* ============================= END OF FILE ======================== */
