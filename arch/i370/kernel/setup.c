/*
 * arch/i370/kernel/setup.c
 *
 * Perform early initialization setup.
 *
 * TODO:
 *  -- Clear out TLB before calling mem_init(). Again, re-IPL seems
 *     to pick up left-over dirt in those. Neither sysclear nor
 *     sysreset clears the TLB.
 */
#include <linux/config.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/blk.h>
#include <linux/console.h>

#include <asm/asm.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/trace.h>
#include <asm/vmdiag.h>
#include <asm/psa.h>

// cmd_line is array of 512 in head.S
extern char cmd_line[512];

// CPUID is the result of a STIDP. Its 8 bytes, and
// must be double-word aligned. Grab extra, and then hack alignment.
unsigned char* CPUID;
unsigned char unaligned_cpuid[16];

// CPU Details
CPU_t cpu_details[NR_CPUS];

char saved_command_line[512];

// Overlay struct PSA onto real address zero.
// The __attribute__ ((section(".psa"))) forces it into its own
// linker section, which is mapped to address zero. See vmlinux.lds
// for that mapping. The first 8 bytes are initialized to the boot
// psw; the goal is to bake these addresses into the binary; they
// must already be set before boot can start.
struct PSA _PSA_  __attribute__ ((section(".psa"))) = {
	/* ipl_psw_new */
	{0x00080000, /* Disabled wait */
	0x80010000}  /* Start of text section */ };

extern void i370_find_devices (unsigned long *memory_start_p,
                               unsigned long *memory_end_p);

void setup_psa(void)
{
	/* Crazy crashes result when low memory has not been cleared
	 * from prior contents (from a hot reboot). sizeof(struct PSA)
	 * is exactly 4KBytes. So we'll just hard-code this. */
	unsigned long *p;
	for (p = (unsigned long*) 0x10; p < (unsigned long*) 0x400; p++)
		*p = 0x0;

	_PSA_.Current = &init_task; /* Current process. */
	_PSA_.cpuadp = _stap();
	_PSA_.cpuadl = _stap();
	_PSA_.cpuno = 1;
	_PSA_.cpumask = 0x0001;
	
	/* Very minimal, in an smp env. it will fail blatantly */
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

	/* align on page boundary ... */
	trace_base = (trc_page_t *)(((int)(*memory_start) + 0xfff));
	trace_base = (trc_page_t *)(((int)(trace_base) & 0x7ffff000));

	/* build circular list */
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
	unsigned long mycpu, i_cpu;
	extern int panic_timeout;
	extern char _etext[], _edata[], _end[];
	// extern char _bss[], _ebss[];

	setup_psa();		/*  */

	/* reboot on panic */	
	panic_timeout = 180;
	
	/* Query & construct CPU info */
	/* On Hercules, at least, CPUID must be double-word aligned. */
	CPUID = (char *) (((((unsigned long) unaligned_cpuid) + 7) >>3) << 3);

	__asm__ __volatile__ (
		"STIDP	%0"
		: "=m" (*CPUID) );

	memset(cpu_details, 0, sizeof(cpu_details));
	mycpu = _stap();
	cpu_details[0].CPU_address = mycpu;
	sprintf(cpu_details[0].CPU_name, "S390%03ld", mycpu);
	cpu_details[0].CPU_status  = 0;
	for (i_cpu = 1; i_cpu < NR_CPUS; i_cpu++) {
		if (i_cpu != mycpu) {
			if (_sigp(i_cpu, SIGPSENS) != 3) {
				cpu_details[i_cpu].CPU_address = i_cpu;
				sprintf(cpu_details[i_cpu].CPU_name,"S390%03ld",i_cpu);
				cpu_details[i_cpu].CPU_status = CPU_stopped;
			}
			else
			{
				cpu_details[i_cpu].CPU_status = CPU_inoprtv;
			}
		}
	}

	/* Set up memory layout. */
	init_task.mm->start_code = PAGE_OFFSET;
	init_task.mm->end_code = (unsigned long) _etext;
	init_task.mm->end_data = (unsigned long) _edata;
//  	init_task.mm->brk = (unsigned long) _end;
  	init_task.mm->brk = (unsigned long) -1;

	/* Save unparsed command line copy for /proc/cmdline */
	strcpy(saved_command_line, cmd_line);
	*cmdline_p = cmd_line;

	printk("Boot command line: %s\n", cmd_line);

	*memory_start_p = (unsigned long) _end;

	/* On bare metal, the first byte is supposed to (always) be zero.
	 * On VM, the first byte is 0xff.
	 * On Hercules, CPUID[0] matches CPUVERID in the config file;
	 * and the rest is CPUSERIAL and CPUMODEL from the Hercules config.
	 */
	if (CPUID[0] != 0x0 && CPUID[0] != 0xff)
		_PSA_.initflag.hercules = 1;

	if (CPUID[0] == 0xff || _i370_hercules_guest_p()) {
		*memory_end_p = VM_Diagnose_Code_60();
	} else {
		/* hardcode the memory size */
		*memory_end_p = 0x2000000;  // 32M
	}

	/* Just search the hardware for devices. */
	i370_find_devices(memory_start_p,memory_end_p);

	/* There's an exception frame as the first thing above the
	   task struct. It will store the PSW. This allows us to
	   pretend we got here after an exception.  The kernel
	   stack starts right after this (and was already set).  */
	init_task.tss.regs->psw = _PSA_.ipl_psw_new;
	init_task.tss.regs->oldregs = 0;
	init_task.tss.regs->caller_sp = 0;

	setup_trace(memory_start_p);

	/* Initialize the page_hash_table to zeros. Failure to do so
	   creates issues when rebooting on VM/Hercules, which give
	   us some left-over junk in this area.  */
	memset(&page_hash_table, 0x0, (PAGE_HASH_SIZE * sizeof(void *)));

#ifdef CONFIG_VT
	conswitchp = &video3270_con;
#endif
}

__initfunc(void i370_initrd(char *str, int *ints))
{
	/* Expecting i370_initrd=<start-addr>,<size in KBytes> */
	if (ints[0] < 2) return;

	initrd_start = ints[1];
	initrd_end = initrd_start + 1024 * ints[2];

	/* Allow large ramdisks. We need 256MB to do reasonable  stuff. */
	extern int rd_size;  /* from drivers/block/rd.c */
	rd_size = 262144;
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

	len += sprintf(len+buffer,"Processor: %08lX\n", (unsigned long) *CPUID);
	for (i_cpu = 0; i_cpu < NR_CPUS; i_cpu++) {
		if (cpu_details[i_cpu].CPU_status != CPU_inoprtv) {
			len += sprintf(len+buffer,"CPU: %s Address: %04ld Status: %08lX\n",
				cpu_details[i_cpu].CPU_name,
				cpu_details[i_cpu].CPU_address,
				cpu_details[i_cpu].CPU_status);
		}
	}

	return len;
}

/* ============================= END OF FILE ======================== */
