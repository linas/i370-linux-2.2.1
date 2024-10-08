/* $Id: pcic.c,v 1.1.1.1 1999/02/08 06:21:18 linas Exp $
 * pcic.c: Sparc/PCI controller support
 *
 * Copyright (C) 1998 V. Roganov and G. Raiko
 *
 * Code is derived from Ultra/PCI PSYCHO controller support, see that
 * for author info.
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/malloc.h>

#include <asm/ebus.h>
#include <asm/sbus.h> /* for sanity check... */

#include <asm/io.h>

#undef PROM_DEBUG
#undef FIXUP_REGS_DEBUG
#undef FIXUP_IRQ_DEBUG
#undef FIXUP_VMA_DEBUG

#ifdef PROM_DEBUG
#define dprintf	prom_printf
#else
#define dprintf printk
#endif

#include <linux/ctype.h>
#include <linux/pci.h>
#include <linux/timex.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/oplib.h>
#include <asm/pcic.h>
#include <asm/timer.h>
#include <asm/uaccess.h>

#ifndef CONFIG_PCI

int pcibios_present(void)
{
	return 0;
}

asmlinkage int sys_pciconfig_read(unsigned long bus,
				  unsigned long dfn,
				  unsigned long off,
				  unsigned long len,
				  unsigned char *buf)
{
	return 0;
}

asmlinkage int sys_pciconfig_write(unsigned long bus,
				   unsigned long dfn,
				   unsigned long off,
				   unsigned long len,
				   unsigned char *buf)
{
	return 0;
}

#else

static struct linux_pcic PCIC;
static struct linux_pcic *pcic = NULL;

static void pci_do_gettimeofday(struct timeval *tv);
static void pci_do_settimeofday(struct timeval *tv);

__initfunc(void pcic_probe(void))
{
	struct linux_prom_registers regs[PROMREG_MAX];
	struct linux_pbm_info* pbm;
	char namebuf[64];
	int node;
	int err;

	if (pcibios_present()) {
		prom_printf("PCIC: called twice!\n");
		prom_halt();
	}

	node = prom_getchild (prom_root_node);
	node = prom_searchsiblings (node, "pci");
	if (node == 0)
		return;
	/*
	 * Map in PCIC register set, config space, and IO base
	 */
	err = prom_getproperty(node, "reg", (char*)regs, sizeof(regs));
	if (err == 0 || err == -1) {
		prom_printf("PCIC: Error, cannot get PCIC registers "
			    "from PROM.\n");
		prom_halt();
	}
	
	pcic = &PCIC;

	pcic->pcic_regs = (unsigned long)sparc_alloc_io(regs[0].phys_addr, NULL,
					      regs[0].reg_size,
					      "PCIC Registers", 0, 0);
	if (!pcic->pcic_regs) {
		prom_printf("PCIC: Error, cannot map PCIC registers.\n");
		prom_halt();
	}

	pcic->pcic_io_phys = regs[1].phys_addr;
	pcic->pcic_io = (unsigned long)sparc_alloc_io(regs[1].phys_addr, NULL,
					    regs[1].reg_size,
					    "PCIC IO Base", 0, 0);
	if (pcic->pcic_io == 0UL) {
		prom_printf("PCIC: Error, cannot map PCIC IO Base.\n");
		prom_halt();
	}

	pcic->pcic_config_space_addr =
			(unsigned long)sparc_alloc_io (regs[2].phys_addr, NULL,
					     regs[2].reg_size * 2,
					     "PCI Config Space Address", 0, 0);
	if (pcic->pcic_config_space_addr == 0UL) {
		prom_printf("PCIC: Error, cannot map" 
			    "PCI Configuration Space Address.\n");
		prom_halt();
	}

	/*
	 * Docs say three least significant bits in address and data
	 * must be the same. Thus, we need adjust size of data.
	 */
	pcic->pcic_config_space_data =
			(unsigned long)sparc_alloc_io (regs[3].phys_addr, NULL,
					     regs[3].reg_size * 2,
					     "PCI Config Space Data", 0, 0);
	if (pcic->pcic_config_space_data == 0UL) {
		prom_printf("PCIC: Error, cannot map" 
			    "PCI Configuration Space Data.\n");
		prom_halt();
	}

	pbm = &pcic->pbm;
	pbm->prom_node = node;
	prom_getstring(node, "name", namebuf, sizeof(namebuf));
	strcpy(pbm->prom_name, namebuf);
}

__initfunc(void pcibios_init(void))
{
	/*
	 * PCIC should be initialized at start of the timer.
	 * So, here we report the presence of PCIC and do some magic passes.
	 */
	if(!pcic)
		return;

	printk("PCIC MAP: config addr=0x%lx; config data=0x%lx, "
	       "regs=0x%lx io=0x%lx\n",
	       pcic->pcic_config_space_addr, pcic->pcic_config_space_data,
	       pcic->pcic_regs, pcic->pcic_io);

	/*
	 * FIXME:
	 *      Switch off IOTLB translation.
	 *      It'll be great to use IOMMU to handle HME's rings
	 *      but we couldn't. Thus, we have to flush CPU cache
	 *      in HME.
	 */
	writeb(PCI_DVMA_CONTROL_IOTLB_DISABLE, 
	       pcic->pcic_regs+PCI_DVMA_CONTROL);

	/*
	 * FIXME:
	 *      Increase mapped size for PCI memory space (DMA access).
	 *      Should be done in that order (size first, address second).
	 *      Why we couldn't set up 4GB and forget about it ?
	 */
	writel(0xF0000000UL, pcic->pcic_regs+PCI_SIZE_0);
	writel(0+PCI_BASE_ADDRESS_SPACE_MEMORY, 
	       pcic->pcic_regs+PCI_BASE_ADDRESS_0);
}

int pcibios_present(void)
{
	return pcic != NULL;
}

__initfunc(static int pdev_to_pnode(struct linux_pbm_info *pbm, 
				    struct pci_dev *pdev))
{
	struct linux_prom_pci_registers regs[PROMREG_MAX];
	int err;
	int node = prom_getchild(pbm->prom_node);

	while(node) {
		err = prom_getproperty(node, "reg", 
				       (char *)&regs[0], sizeof(regs));
		if(err != 0 && err != -1) {
			unsigned long devfn = (regs[0].which_io >> 8) & 0xff;
			if(devfn == pdev->devfn)
				return node; /* Match */
		}
		node = prom_getsibling(node);
	}
	return 0;
}

static inline struct pcidev_cookie *pci_devcookie_alloc(void)
{
	return kmalloc(sizeof(struct pcidev_cookie), GFP_ATOMIC);
}


static void pcic_map_pci_device (struct pci_dev *dev) {
	int node, pcinode;
	int i, j;

	/* Is any valid address present ? */
	i = 0;
	for(j = 0; j < 6; j++)
		if (dev->base_address[j]) i++;
	if (!i) return; /* nothing to do */

	/*
	 * find related address and get it's window length
	 */
	pcinode = prom_getchild(prom_root_node);
	pcinode = prom_searchsiblings(pcinode, "pci");
	if (!pcinode)
		panic("PCIC: failed to locate 'pci' node");


	for (node = prom_getchild(pcinode); node;
	     node = prom_getsibling(node)) {
		struct linux_prom_pci_assigned_addresses addrs[6];
		int addrlen = prom_getproperty(node,"assigned-addresses",
					       (char*)addrs, sizeof(addrs));
		if (addrlen == -1)
			continue;

		addrlen /= sizeof(struct linux_prom_pci_assigned_addresses);
		for (i = 0; i < addrlen; i++ )
		    for (j = 0; j < 6; j++) {
			if (!dev->base_address[j] || !addrs[i].phys_lo)
				continue;
			if (addrs[i].phys_lo == dev->base_address[j]) {
			    unsigned long address = dev->base_address[j];
			    int length  = addrs[i].size_lo;
			    char namebuf[128] = { 0, };
			    unsigned long mapaddr, addrflags;
	    
			    prom_getstring(node, "name",
					   namebuf,  sizeof(namebuf));

			    /* FIXME:
			     *      failure in allocation too large space
			     */
			    if (length > 0x200000) {
				length = 0x200000;
				prom_printf("PCIC: map window for device '%s' "
					    "reduced to 2MB !\n", namebuf);
			    }

			    /*
			     *  Be careful with MEM/IO address flags
			     */
			    if ((address & PCI_BASE_ADDRESS_SPACE) ==
				 PCI_BASE_ADDRESS_SPACE_IO) {
				mapaddr = address & PCI_BASE_ADDRESS_IO_MASK;
			    } else {
				mapaddr = address & PCI_BASE_ADDRESS_MEM_MASK;
			    }
			    addrflags = address ^ mapaddr;

			    dev->base_address[j] =
				(unsigned long)sparc_alloc_io(address, 0, 
							      length,
							      namebuf, 0, 0);
			    if ( dev->base_address[j] == 0 )
				panic("PCIC: failed make mapping for "
				      "pci device '%s' with address %lx\n",
				       namebuf, address);

			    dev->base_address[j] ^= addrflags;
			    return;
			}
		}
	}

	panic("PCIC: unable to locate prom node for pci device (%x,%x) \n",
	      dev->device, dev->vendor);
}

/*
 * Assign IO space for a device.
 * This is a chance for devices which have the same IO and Mem Space to
 * fork access to IO and Mem.
 *
 * Now, we assume there is one such device only (IGA 1682) but code below
 * should work in cases when space of all such devices is less then 16MB.
 */
unsigned long pcic_alloc_io( unsigned long* addr )
{
	unsigned long paddr = *addr;
	unsigned long offset;

	if(pcic->pcic_mapped_io == 0) {
		pcic->pcic_mapped_io = paddr & ~(PCI_SPACE_SIZE-1) ;
		writeb((pcic->pcic_mapped_io>>24) & 0xff, 
		       pcic->pcic_regs+PCI_PIBAR);
		writeb((pcic->pcic_io_phys>>24) & PCI_SIBAR_ADDRESS_MASK,
		       pcic->pcic_regs+PCI_SIBAR);
		writeb(PCI_ISIZE_16M, pcic->pcic_regs+PCI_ISIZE);
	}
	if(paddr < pcic->pcic_mapped_io ||
	   paddr > pcic->pcic_mapped_io + PCI_SPACE_SIZE)
		return 0;
	offset = paddr - pcic->pcic_mapped_io;
	*addr = pcic->pcic_io_phys + offset;
	return pcic->pcic_io + offset;
}

/*
 * Stolen from both i386 and sparc64 branch 
 */
__initfunc(void pcibios_fixup(void))
{
  struct pci_dev *dev;
  int i, has_io, has_mem;
  unsigned short cmd;

	if(pcic == NULL) {
		prom_printf("PCI: Error, PCIC not found.\n");
		prom_halt();
	}

	for (dev = pci_devices; dev; dev=dev->next) {
		/*
		 * Comment from i386 branch:
		 *     There are buggy BIOSes that forget to enable I/O and memory
		 *     access to PCI devices. We try to fix this, but we need to
		 *     be sure that the BIOS didn't forget to assign an address
		 *     to the device. [mj]
		 * OBP is a case of such BIOS :-)
		 */
		has_io = has_mem = 0;
		for(i=0; i<6; i++) {
			unsigned long a = dev->base_address[i];
			if (a & PCI_BASE_ADDRESS_SPACE_IO) {
				has_io = 1;
			} else if (a & PCI_BASE_ADDRESS_MEM_MASK)
				has_mem = 1;
		}
		pci_read_config_word(dev, PCI_COMMAND, &cmd);
		if (has_io && !(cmd & PCI_COMMAND_IO)) {
			printk("PCI: Enabling I/O for device %02x:%02x\n",
				dev->bus->number, dev->devfn);
			cmd |= PCI_COMMAND_IO;
			pci_write_config_word(dev, PCI_COMMAND, cmd);
		}
		if (has_mem && !(cmd & PCI_COMMAND_MEMORY)) {
			printk("PCI: Enabling memory for device %02x:%02x\n",
				dev->bus->number, dev->devfn);
			cmd |= PCI_COMMAND_MEMORY;
			pci_write_config_word(dev, PCI_COMMAND, cmd);
		}    

		/* cookies */
		{
			struct pcidev_cookie *pcp;
			struct linux_pbm_info* pbm = &pcic->pbm;
			int node = pdev_to_pnode(pbm, dev);

			if(node == 0)
				node = -1;
			pcp = pci_devcookie_alloc();
			pcp->pbm = pbm;
			pcp->prom_node = node;
			dev->sysdata = pcp;
		}

		/* memory mapping */
		if (!(dev->vendor == PCI_VENDOR_ID_SUN &&
		      dev->device == PCI_DEVICE_ID_SUN_EBUS)) {
			pcic_map_pci_device(dev);
		}

		/* irq */
#define SETIRQ(vend,devid,irqn) \
	if (dev->vendor==vend && dev->device==devid) dev->irq = irqn;

		SETIRQ(PCI_VENDOR_ID_SUN,PCI_DEVICE_ID_SUN_HAPPYMEAL,3);
	}
	ebus_init();
}

/* Makes compiler happy */
static volatile int pcic_timer_dummy;

static void pcic_clear_clock_irq(void)
{
	pcic_timer_dummy = readl(pcic->pcic_regs+PCI_SYS_LIMIT);
}

static void pcic_timer_handler (int irq, void *h, struct pt_regs *regs)
{
	pcic_clear_clock_irq();
	do_timer(regs);
}

#define USECS_PER_JIFFY  10000  /* We have 100HZ "standard" timer for sparc */
#define TICK_TIMER_LIMIT ((100*1000000/4)/100)

__initfunc(void pci_time_init(void))
{
	unsigned long v;
	int timer_irq, irq;

	do_get_fast_time = pci_do_gettimeofday;
	/* A hack until do_gettimeofday prototype is moved to arch specific headers
	   and btfixupped. Patch do_gettimeofday with ba pci_do_gettimeofday; nop */
	((unsigned int *)do_gettimeofday)[0] = 
		0x10800000 | ((((unsigned long)pci_do_gettimeofday - (unsigned long)do_gettimeofday) >> 2) & 0x003fffff);
	((unsigned int *)do_gettimeofday)[1] =
		0x01000000;
	BTFIXUPSET_CALL(bus_do_settimeofday, pci_do_settimeofday, BTFIXUPCALL_NORM);
	btfixup();

	writel (TICK_TIMER_LIMIT, pcic->pcic_regs+PCI_SYS_LIMIT);
	/* PROM should set appropriate irq */
	v = readb(pcic->pcic_regs+PCI_COUNTER_IRQ);
	timer_irq = PCI_COUNTER_IRQ_SYS(v);
	writel (PCI_COUNTER_IRQ_SET(timer_irq, 0),
		pcic->pcic_regs+PCI_COUNTER_IRQ);
	irq = request_irq(timer_irq, pcic_timer_handler,
			  (SA_INTERRUPT | SA_STATIC_ALLOC), "timer", NULL);
	if (irq) {
		prom_printf("time_init: unable to attach IRQ%d\n", timer_irq);
		prom_halt();
	}
	__sti();
}

static __inline__ unsigned long do_gettimeoffset(void)
{
	unsigned long offset = 0;

	/* 
	 * We devide all to 100
	 * to have microsecond resolution and to avoid overflow
	 */
	unsigned long count = 
	    readl(pcic->pcic_regs+PCI_SYS_COUNTER) & ~PCI_SYS_COUNTER_OVERFLOW;
	count = ((count/100)*USECS_PER_JIFFY) / (TICK_TIMER_LIMIT/100);

	if(test_bit(TIMER_BH, &bh_active))
		offset = 1000000;
	return offset + count;
}

extern volatile unsigned long lost_ticks;

static void pci_do_gettimeofday(struct timeval *tv)
{
 	unsigned long flags;

	save_and_cli(flags);
	*tv = xtime;
	tv->tv_usec += do_gettimeoffset();

	/*
	 * xtime is atomically updated in timer_bh. lost_ticks is
	 * nonzero if the timer bottom half hasnt executed yet.
	 */
	if (lost_ticks)
		tv->tv_usec += USECS_PER_JIFFY;

	restore_flags(flags);

	if (tv->tv_usec >= 1000000) {
		tv->tv_usec -= 1000000;
		tv->tv_sec++;
	}       
}

static void pci_do_settimeofday(struct timeval *tv)
{
	cli();
	tv->tv_usec -= do_gettimeoffset();
	if(tv->tv_usec < 0) {
		tv->tv_usec += 1000000;
		tv->tv_sec--;
	}
	xtime = *tv;
	time_state = TIME_BAD;
	time_maxerror = 0x70000000;
	time_esterror = 0x70000000;
	sti();
}

#if 0
static void watchdog_reset() {
	writeb(0, pcic->pcic_regs+PCI_SYS_STATUS);
}
#endif

#define CONFIG_CMD(bus, device_fn, where) (0x80000000 | (((unsigned int)bus) << 16) | (((unsigned int)device_fn) << 8) | (where & ~3))

int pcibios_read_config_byte(unsigned char bus, unsigned char device_fn,
			     unsigned char where, unsigned char *value)
{
	unsigned int v;

	pcibios_read_config_dword (bus, device_fn, where&~3, &v);
	*value = 0xff & (v >> (8*(where & 3)));
	return PCIBIOS_SUCCESSFUL;
}

int pcibios_read_config_word (unsigned char bus,
			      unsigned char device_fn, 
			      unsigned char where, unsigned short *value)
{
	unsigned int v;
	if (where&1) return PCIBIOS_BAD_REGISTER_NUMBER;
  
	pcibios_read_config_dword (bus, device_fn, where&~3, &v);
	*value = 0xffff & (v >> (8*(where & 3)));
	return PCIBIOS_SUCCESSFUL;
}

int pcibios_read_config_dword (unsigned char bus, unsigned char device_fn,
			       unsigned char where, unsigned int *value)
{
	unsigned long flags;
	if (where&3) return PCIBIOS_BAD_REGISTER_NUMBER;
	if (bus != 0 || 
	    (device_fn != 0 && device_fn != 1 && device_fn != 0x80)) {
		*value = 0xffffffff;
		return PCIBIOS_SUCCESSFUL;
	}

	/* FIXME: IGA haven't got high config memory addresses !!! */
	if (device_fn == 0x80 && where > PCI_INTERRUPT_LINE) {
		*value = 0xffffffff;
		return PCIBIOS_SUCCESSFUL;
	}

	save_and_cli(flags);
	writel(CONFIG_CMD(bus,device_fn,where), pcic->pcic_config_space_addr);
	*value = readl(pcic->pcic_config_space_data + (where&4));
	restore_flags(flags);
	return PCIBIOS_SUCCESSFUL;
}
    
int pcibios_write_config_byte (unsigned char bus, unsigned char devfn,
			       unsigned char where, unsigned char value)
{
	unsigned int v;

	pcibios_read_config_dword (bus, devfn, where&~3, &v);
	v = (v & ~(0xff << (8*(where&3)))) | 
	    ((0xff&(unsigned)value) << (8*(where&3)));
	return pcibios_write_config_dword (bus, devfn, where&~3, v);
}

int pcibios_write_config_word (unsigned char bus, unsigned char devfn,
			       unsigned char where, unsigned short value)
{
	unsigned int v;
	if (where&1) return PCIBIOS_BAD_REGISTER_NUMBER;

	pcibios_read_config_dword (bus, devfn, where&~3, &v);
	v = (v & ~(0xffff << (8*(where&3)))) | 
	    ((0xffff&(unsigned)value) << (8*(where&3)));
	return pcibios_write_config_dword (bus, devfn, where&~3, v);
}
  
int pcibios_write_config_dword (unsigned char bus, unsigned char devfn,
				unsigned char where, unsigned int value)
{
	unsigned long flags;
	if ((where&3) || bus != 0 || (devfn != 0 && devfn != 1 && devfn != 0x80))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	save_and_cli(flags);
	writel(CONFIG_CMD(bus,devfn,where),pcic->pcic_config_space_addr);
	writel(value, pcic->pcic_config_space_data + (where&4));
	restore_flags(flags);
	return PCIBIOS_SUCCESSFUL;
}

__initfunc(char *pcibios_setup(char *str))
{
	return str;
}

/*
 *  Following code added to handle extra PCI-related system calls 
 */
asmlinkage int sys_pciconfig_read(unsigned long bus,
				  unsigned long dfn,
				  unsigned long off,
				  unsigned long len,
				  unsigned char *buf)
{
	unsigned char ubyte;
	unsigned short ushort;
	unsigned int uint;
	int err = 0;

	if(!suser())
		return -EPERM;

	lock_kernel();
	switch(len) {
	case 1:
		pcibios_read_config_byte(bus, dfn, off, &ubyte);
		put_user(ubyte, (unsigned char *)buf);
		break;
	case 2:
		pcibios_read_config_word(bus, dfn, off, &ushort);
		put_user(ushort, (unsigned short *)buf);
		break;
	case 4:
		pcibios_read_config_dword(bus, dfn, off, &uint);
		put_user(uint, (unsigned int *)buf);
		break;

	default:
		err = -EINVAL;
		break;
	};
	unlock_kernel();

	return err;
}
   
asmlinkage int sys_pciconfig_write(unsigned long bus,
				   unsigned long dfn,
				   unsigned long off,
				   unsigned long len,
				   unsigned char *buf)
{
	unsigned char ubyte;
	unsigned short ushort;
	unsigned int uint;
	int err = 0;

	if(!suser())
		return -EPERM;

	lock_kernel();
	switch(len) {
	case 1:
		err = get_user(ubyte, (unsigned char *)buf);
		if(err)
			break;
		pcibios_write_config_byte(bus, dfn, off, ubyte);
		break;

	case 2:
		err = get_user(ushort, (unsigned short *)buf);
		if(err)
			break;
		pcibios_write_config_byte(bus, dfn, off, ushort);
		break;

	case 4:
		err = get_user(uint, (unsigned int *)buf);
		if(err)
			break;
		pcibios_write_config_byte(bus, dfn, off, uint);
		break;

	default:
		err = -EINVAL;
		break;

	};
	unlock_kernel();

	return err;
}			   

static inline unsigned long get_irqmask(int irq_nr)
{
	return 1 << irq_nr;
}

static inline char *pcic_irq_itoa(unsigned int irq)
{
	static char buff[16];
	sprintf(buff, "%d", irq);
	return buff;
}

static void pcic_disable_irq(unsigned int irq_nr)
{
	unsigned long mask, flags;

	mask = get_irqmask(irq_nr);
	save_and_cli(flags);
	writel(mask, pcic->pcic_regs+PCI_SYS_INT_TARGET_MASK_SET);
	restore_flags(flags);
}

static void pcic_enable_irq(unsigned int irq_nr)
{
	unsigned long mask, flags;

	mask = get_irqmask(irq_nr);
	save_and_cli(flags);
	writel(mask, pcic->pcic_regs+PCI_SYS_INT_TARGET_MASK_CLEAR);
	restore_flags(flags);
}

static void pcic_clear_profile_irq(int cpu)
{
	printk("PCIC: unimplemented code: FILE=%s LINE=%d", __FILE__, __LINE__);
}

static void pcic_load_profile_irq(int cpu, unsigned int limit)
{
	printk("PCIC: unimplemented code: FILE=%s LINE=%d", __FILE__, __LINE__);
}

/* We assume the caller is local cli()'d when these are called, or else
 * very bizarre behavior will result.
 */
static void pcic_disable_pil_irq(unsigned int pil)
{
	writel(get_irqmask(pil), pcic->pcic_regs+PCI_SYS_INT_TARGET_MASK_SET);
}

static void pcic_enable_pil_irq(unsigned int pil)
{
	writel(get_irqmask(pil), pcic->pcic_regs+PCI_SYS_INT_TARGET_MASK_CLEAR);
}

__initfunc(void sun4m_pci_init_IRQ(void))
{
	BTFIXUPSET_CALL(enable_irq, pcic_enable_irq, BTFIXUPCALL_NORM);
	BTFIXUPSET_CALL(disable_irq, pcic_disable_irq, BTFIXUPCALL_NORM);
	BTFIXUPSET_CALL(enable_pil_irq, pcic_enable_pil_irq, BTFIXUPCALL_NORM);
	BTFIXUPSET_CALL(disable_pil_irq, pcic_disable_pil_irq, BTFIXUPCALL_NORM);
	BTFIXUPSET_CALL(clear_clock_irq, pcic_clear_clock_irq, BTFIXUPCALL_NORM);
	BTFIXUPSET_CALL(clear_profile_irq, pcic_clear_profile_irq, BTFIXUPCALL_NORM);
	BTFIXUPSET_CALL(load_profile_irq, pcic_load_profile_irq, BTFIXUPCALL_NORM);
	BTFIXUPSET_CALL(__irq_itoa, pcic_irq_itoa, BTFIXUPCALL_NORM);
}

__initfunc(void pcibios_fixup_bus(struct pci_bus *bus))
{
}

#endif
