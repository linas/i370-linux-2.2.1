/*********************************************************************
 *
 * msnd.c - Driver Base
 *
 * Turtle Beach MultiSound Sound Card Driver for Linux
 *
 * Copyright (C) 1998 Andrew Veliath
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: msnd.c,v 1.1 1999/02/08 06:20:27 linas Exp $
 *
 ********************************************************************/

#include <linux/version.h>
#if LINUX_VERSION_CODE < 0x020101
#  define LINUX20
#endif
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/malloc.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/mm.h>
#ifdef LINUX20
#  include <linux/major.h>
#  include <linux/fs.h>
#  include <linux/sound.h>
#  include <asm/segment.h>
#  include "sound_config.h"
#else
#  include <linux/init.h>
#  include <asm/io.h>
#  include <asm/uaccess.h>
#  include <asm/spinlock.h>
#endif
#include <asm/irq.h>
#include "msnd.h"

#define LOGNAME			"msnd"

#define MSND_MAX_DEVS		4

static multisound_dev_t		*devs[MSND_MAX_DEVS];
static int			num_devs;

int msnd_register(multisound_dev_t *dev)
{
	int i;

	for (i = 0; i < MSND_MAX_DEVS; ++i)
		if (devs[i] == NULL)
			break;

	if (i == MSND_MAX_DEVS)
		return -ENOMEM;
	
	devs[i] = dev;
	++num_devs;

	MOD_INC_USE_COUNT;

	return 0;
}

void msnd_unregister(multisound_dev_t *dev)
{
	int i;

	for (i = 0; i < MSND_MAX_DEVS; ++i)
		if (devs[i] == dev)
			break;

	if (i == MSND_MAX_DEVS) {
		printk(KERN_WARNING LOGNAME ": Unregistering unknown device\n");
		return;
	}

	devs[i] = NULL;
	--num_devs;

	MOD_DEC_USE_COUNT;
}

int msnd_get_num_devs(void)
{
	return num_devs;
}

multisound_dev_t *msnd_get_dev(int j)
{
	int i;

	for (i = 0; i < MSND_MAX_DEVS && j; ++i)
		if (devs[i] != NULL)
			--j;
	
	if (i == MSND_MAX_DEVS || j != 0)
		return NULL;

	return devs[i];
}

void msnd_fifo_init(msnd_fifo *f)
{
	f->data = NULL;
}

void msnd_fifo_free(msnd_fifo *f)
{
	if (f->data) {
		vfree(f->data);
		f->data = NULL;
	}
}

int msnd_fifo_alloc(msnd_fifo *f, size_t n)
{
	msnd_fifo_free(f);
	f->data = (char *)vmalloc(n);
	f->n = n;
	f->tail = 0;
	f->head = 0;
	f->len = 0;

	if (!f->data)
		return -ENOMEM;

	return 0;
}

void msnd_fifo_make_empty(msnd_fifo *f)
{
	f->len = f->tail = f->head = 0;
}

int msnd_fifo_write(msnd_fifo *f, const char *buf, size_t len, int user)
{
	int count = 0;

	if (f->len == f->n)
		return 0;
	
	while ((count < len) && (f->len != f->n)) {
		
		int nwritten;
		
		if (f->head <= f->tail) {
			nwritten = len - count;
			if (nwritten > f->n - f->tail)
				nwritten = f->n - f->tail;
		}
		else {
			nwritten = f->head - f->tail;
			if (nwritten > len - count)
				nwritten = len - count;
		}

		if (user) {
			if (copy_from_user(f->data + f->tail, buf, nwritten))
				return -EFAULT;
		} else
			memcpy(f->data + f->tail, buf, nwritten);

		count += nwritten;
		buf += nwritten;
		f->len += nwritten;
		f->tail += nwritten;
		f->tail %= f->n;
	}
	
	return count;
}

int msnd_fifo_read(msnd_fifo *f, char *buf, size_t len, int user)
{
	int count = 0;

	if (f->len == 0)
		return f->len;
	
	while ((count < len) && (f->len > 0)) {
		
		int nread;
		
		if (f->tail <= f->head) {
			nread = len - count;
			if (nread > f->n - f->head)
				nread = f->n - f->head;
		}
		else {
			nread = f->tail - f->head;
			if (nread > len - count)
				nread = len - count;
		}
		
		if (user) {
			if (copy_to_user(buf, f->data + f->head, nread))
				return -EFAULT;
		} else
			memcpy(buf, f->data + f->head, nread);
		
		count += nread;
		buf += nread;
		f->len -= nread;
		f->head += nread;
		f->head %= f->n;
	}
	
	return count;
}

int msnd_wait_TXDE(multisound_dev_t *dev)
{
	register unsigned int io = dev->io;
	register int timeout = 1000;
    
	while(timeout-- > 0)
		if (inb(io + HP_ISR) & HPISR_TXDE)
			return 0;

	return -EIO;
}

int msnd_wait_HC0(multisound_dev_t *dev)
{
	register unsigned int io = dev->io;
	register int timeout = 1000;

	while(timeout-- > 0)
		if (!(inb(io + HP_CVR) & HPCVR_HC))
			return 0;

	return -EIO;
}

int msnd_send_dsp_cmd(multisound_dev_t *dev, BYTE cmd)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	if (msnd_wait_HC0(dev) == 0) {
		outb(cmd, dev->io + HP_CVR);
		spin_unlock_irqrestore(&dev->lock, flags);
		return 0;
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	printk(KERN_DEBUG LOGNAME ": Send DSP command timeout\n");
	
	return -EIO;
}

int msnd_send_word(multisound_dev_t *dev, unsigned char high,
		   unsigned char mid, unsigned char low)
{
	register unsigned int io = dev->io;

	if (msnd_wait_TXDE(dev) == 0) {
		outb(high, io + HP_TXH);
		outb(mid, io + HP_TXM);
		outb(low, io + HP_TXL);
		return 0;
	}

	printk(KERN_DEBUG LOGNAME ": Send host word timeout\n");

	return -EIO;
}

int msnd_upload_host(multisound_dev_t *dev, char *bin, int len)
{
	int i;

	if (len % 3 != 0) {
		printk(KERN_WARNING LOGNAME ": Upload host data not multiple of 3!\n");		
		return -EINVAL;
	}

	for (i = 0; i < len; i += 3)
		if (msnd_send_word(dev, bin[i], bin[i + 1], bin[i + 2]) != 0)
			return -EIO;

	inb(dev->io + HP_RXL);
	inb(dev->io + HP_CVR);

	return 0;
}

int msnd_enable_irq(multisound_dev_t *dev)
{
	unsigned long flags;

	if (dev->irq_ref++)
		return 0;

	printk(KERN_DEBUG LOGNAME ": Enabling IRQ\n");
	
	spin_lock_irqsave(&dev->lock, flags);
	if (msnd_wait_TXDE(dev) == 0) {
		outb(inb(dev->io + HP_ICR) | HPICR_TREQ, dev->io + HP_ICR);
		if (dev->type == msndClassic)
			outb(dev->irqid, dev->io + HP_IRQM);
		outb(inb(dev->io + HP_ICR) & ~HPICR_TREQ, dev->io + HP_ICR);
		outb(inb(dev->io + HP_ICR) | HPICR_RREQ, dev->io + HP_ICR);
		enable_irq(dev->irq);
		spin_unlock_irqrestore(&dev->lock, flags);
		return 0;
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	printk(KERN_DEBUG LOGNAME ": Enable IRQ failed\n");

	return -EIO;
}

int msnd_disable_irq(multisound_dev_t *dev)
{
	unsigned long flags;

	if (--dev->irq_ref > 0)
		return 0;

	if (dev->irq_ref < 0)
		printk(KERN_DEBUG LOGNAME ": IRQ ref count is %d\n", dev->irq_ref);

	printk(KERN_DEBUG LOGNAME ": Disabling IRQ\n");

	spin_lock_irqsave(&dev->lock, flags);
	if (msnd_wait_TXDE(dev) == 0) {
		outb(inb(dev->io + HP_ICR) & ~HPICR_RREQ, dev->io + HP_ICR);
		if (dev->type == msndClassic)
			outb(HPIRQ_NONE, dev->io + HP_IRQM);
		disable_irq(dev->irq);
		spin_unlock_irqrestore(&dev->lock, flags);
		return 0;
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	printk(KERN_DEBUG LOGNAME ": Disable IRQ failed\n");

	return -EIO;
}

#ifndef LINUX20
EXPORT_SYMBOL(msnd_register);
EXPORT_SYMBOL(msnd_unregister);
EXPORT_SYMBOL(msnd_get_num_devs);
EXPORT_SYMBOL(msnd_get_dev);

EXPORT_SYMBOL(msnd_fifo_init);
EXPORT_SYMBOL(msnd_fifo_free);
EXPORT_SYMBOL(msnd_fifo_alloc);
EXPORT_SYMBOL(msnd_fifo_make_empty);
EXPORT_SYMBOL(msnd_fifo_write);
EXPORT_SYMBOL(msnd_fifo_read);

EXPORT_SYMBOL(msnd_wait_TXDE);
EXPORT_SYMBOL(msnd_wait_HC0);
EXPORT_SYMBOL(msnd_send_dsp_cmd);
EXPORT_SYMBOL(msnd_send_word);
EXPORT_SYMBOL(msnd_upload_host);

EXPORT_SYMBOL(msnd_enable_irq);
EXPORT_SYMBOL(msnd_disable_irq);
#endif

#ifdef MODULE
MODULE_AUTHOR				("Andrew Veliath <andrewtv@usa.net>");
MODULE_DESCRIPTION			("Turtle Beach MultiSound Driver Base");

int init_module(void)
{
	return 0;
}

void cleanup_module(void)
{
}
#endif
