/* $Id: icn.h,v 1.1.1.1 1999/02/08 06:20:42 linas Exp $

 * ISDN lowlevel-module for the ICN active ISDN-Card.
 *
 * Copyright 1994 by Fritz Elfert (fritz@wuemaus.franken.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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
 * $Log: icn.h,v $
 * Revision 1.1.1.1  1999/02/08 06:20:42  linas
 * stock linux 2.2.1 kernel
 *
 * Revision 1.28  1997/10/10 15:56:18  fritz
 * New HL<->LL interface:
 *   New BSENT callback with nr. of bytes included.
 *   Sending without ACK.
 *
 * Revision 1.27  1997/10/01 09:21:56  fritz
 * Removed old compatibility stuff for 2.0.X kernels.
 * From now on, this code is for 2.1.X ONLY!
 * Old stuff is still in the separate branch.
 *
 * Revision 1.26  1997/02/14 12:23:16  fritz
 * Added support for new insmod parameter handling.
 *
 * Revision 1.25  1997/02/10 10:10:31  fritz
 * Changes for Kernel 2.1.X compatibility.
 * Enhanced initialization, can recover from
 * misconfiguration by other autoprobing drivers.
 *
 * Revision 1.24  1997/01/29 22:34:46  fritz
 * Cleanup, Corrected D64S setup of 2nd channel.
 *
 * Revision 1.23  1996/12/17 18:47:55  tsbogend
 * changed timeouts from absolute numbers to HZ based values
 *
 * Revision 1.22  1996/11/13 02:37:33  fritz
 * Added delay include.
 *
 * Revision 1.21  1996/08/29 20:35:57  fritz
 * Speed up B-Channel polling interval.
 *
 * Revision 1.20  1996/06/24 17:20:37  fritz
 * Bugfixes in pollbchan_send():
 *   - Using lock field of skbuff breaks networking.
 *   - Added channel locking
 *   - changed dequeuing scheme.
 * Eliminated misc. compiler warnings.
 *
 * Revision 1.19  1996/06/06 13:58:35  fritz
 * Changed code to be architecture independent
 *
 * Revision 1.18  1996/06/03 19:59:30  fritz
 * Removed include of config.h
 *
 * Revision 1.17  1996/05/18 00:47:04  fritz
 * Removed callback debug code.
 *
 * Revision 1.16  1996/05/17 15:46:43  fritz
 * Removed own queue management.
 * Changed queue management to use sk_buffs.
 *
 * Revision 1.15  1996/05/02 04:01:57  fritz
 * Removed ICN_MAXCARDS
 *
 * Revision 1.14  1996/05/02 00:40:29  fritz
 * Major rewrite to support more than one card
 * with a single module.
 * Support for new firmware.
 *
 * Revision 1.13  1996/04/20 16:51:41  fritz
 * Increased status buffer.
 * Misc. typos
 *
 * Revision 1.12  1996/01/22 05:01:22  fritz
 * Revert to GPL.
 *
 * Revision 1.11  1995/12/18  18:25:00  fritz
 * Support for ICN-2B Cards.
 * Change for supporting user-settable service-octet.
 *
 * Revision 1.10  1995/10/29  21:43:10  fritz
 * Added support for leased lines.
 *
 * Revision 1.9  1995/04/23  13:42:10  fritz
 * Added some constants for distinguishing 1TR6 and DSS1
 *
 * Revision 1.8  1995/03/25  23:18:55  fritz
 * Changed ICN_PORTLEN to reflect correct number of ports.
 *
 * Revision 1.7  1995/03/15  12:52:06  fritz
 * Some cleanup
 *
 * Revision 1.6  1995/02/20  03:49:22  fritz
 * Fixed ICN_MAX_SQUEUE to correctly reflect outstanding bytes, not number
 * of buffers.
 *
 * Revision 1.5  1995/01/29  23:36:50  fritz
 * Minor cleanup.
 *
 * Revision 1.4  1995/01/09  07:41:20  fritz
 * Added GPL-Notice
 *
 * Revision 1.3  1995/01/04  05:14:20  fritz
 * removed include of linux/asm/string.h for compiling with Linux 1.1.76
 *
 * Revision 1.2  1995/01/02  02:15:57  fritz
 * Misc. Bugfixes
 *
 * Revision 1.1  1994/12/14  18:02:38  fritz
 * Initial revision
 *
 */

#ifndef icn_h
#define icn_h

#define ICN_IOCTL_SETMMIO   0
#define ICN_IOCTL_GETMMIO   1
#define ICN_IOCTL_SETPORT   2
#define ICN_IOCTL_GETPORT   3
#define ICN_IOCTL_LOADBOOT  4
#define ICN_IOCTL_LOADPROTO 5
#define ICN_IOCTL_LEASEDCFG 6
#define ICN_IOCTL_GETDOUBLE 7
#define ICN_IOCTL_DEBUGVAR  8
#define ICN_IOCTL_ADDCARD   9

/* Struct for adding new cards */
typedef struct icn_cdef {
	int port;
	char id1[10];
	char id2[10];
} icn_cdef;

#if defined(__KERNEL__) || defined(__DEBUGVAR__)

#ifdef __KERNEL__
/* Kernel includes */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <asm/segment.h>
#include <asm/io.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/malloc.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/isdnif.h>

#endif                          /* __KERNEL__ */

/* some useful macros for debugging */
#ifdef ICN_DEBUG_PORT
#define OUTB_P(v,p) {printk(KERN_DEBUG "icn: outb_p(0x%02x,0x%03x)\n",v,p); outb_p(v,p);}
#else
#define OUTB_P outb
#endif

/* Defaults for Port-Address and shared-memory */
#define ICN_BASEADDR 0x320
#define ICN_PORTLEN (0x04)
#define ICN_MEMADDR 0x0d0000

#define ICN_FLAGS_B1ACTIVE 1    /* B-Channel-1 is open                     */
#define ICN_FLAGS_B2ACTIVE 2    /* B-Channel-2 is open                     */
#define ICN_FLAGS_RUNNING  4    /* Cards driver activated                  */
#define ICN_FLAGS_RBTIMER  8    /* cyclic scheduling of B-Channel-poll     */

#define ICN_BOOT_TIMEOUT1  (HZ) /* Delay for Boot-download (jiffies)       */
#define ICN_CHANLOCK_DELAY (HZ/10)	/* Delay for Channel-mapping (jiffies)     */

#define ICN_TIMER_BCREAD (HZ/100)	/* B-Channel poll-cycle                    */
#define ICN_TIMER_DCREAD (HZ/2) /* D-Channel poll-cycle                    */

#define ICN_CODE_STAGE1 4096    /* Size of bootcode                        */
#define ICN_CODE_STAGE2 65536   /* Size of protocol-code                   */

#define ICN_MAX_SQUEUE 8000     /* Max. outstanding send-data (2* hw-buf.) */
#define ICN_FRAGSIZE (250)      /* Max. size of send-fragments             */
#define ICN_BCH 2               /* Number of supported channels per card   */

/* type-definitions for accessing the mmap-io-areas */

#define SHM_DCTL_OFFSET (0)     /* Offset to data-controlstructures in shm */
#define SHM_CCTL_OFFSET (0x1d2) /* Offset to comm-controlstructures in shm */
#define SHM_CBUF_OFFSET (0x200) /* Offset to comm-buffers in shm           */
#define SHM_DBUF_OFFSET (0x2000)	/* Offset to data-buffers in shm           */

/*
 * Layout of card's data buffers
 */
typedef struct {
	unsigned char length;   /* Bytecount of fragment (max 250)    */
	unsigned char endflag;  /* 0=last frag., 0xff=frag. continued */
	unsigned char data[ICN_FRAGSIZE];	/* The data                           */
	/* Fill to 256 bytes */
	char unused[0x100 - ICN_FRAGSIZE - 2];
} frag_buf;

/*
 * Layout of card's shared memory
 */
typedef union {
	struct {
		unsigned char scns;	/* Index to free SendFrag.             */
		unsigned char scnr;	/* Index to active SendFrag   READONLY */
		unsigned char ecns;	/* Index to free RcvFrag.     READONLY */
		unsigned char ecnr;	/* Index to valid RcvFrag              */
		char unused[6];
		unsigned short fuell1;	/* Internal Buf Bytecount              */
	} data_control;
	struct {
		char unused[SHM_CCTL_OFFSET];
		unsigned char iopc_i;	/* Read-Ptr Status-Queue      READONLY */
		unsigned char iopc_o;	/* Write-Ptr Status-Queue              */
		unsigned char pcio_i;	/* Write-Ptr Command-Queue             */
		unsigned char pcio_o;	/* Read-Ptr Command Queue     READONLY */
	} comm_control;
	struct {
		char unused[SHM_CBUF_OFFSET];
		unsigned char pcio_buf[0x100];	/* Ring-Buffer Command-Queue           */
		unsigned char iopc_buf[0x100];	/* Ring-Buffer Status-Queue            */
	} comm_buffers;
	struct {
		char unused[SHM_DBUF_OFFSET];
		frag_buf receive_buf[0x10];
		frag_buf send_buf[0x10];
	} data_buffers;
} icn_shmem;

/*
 * Per card driver data
 */
typedef struct icn_card {
	struct icn_card *next;  /* Pointer to next device struct    */
	struct icn_card *other; /* Pointer to other card for ICN4B  */
	unsigned short port;    /* Base-port-address                */
	int myid;               /* Driver-Nr. assigned by linklevel */
	int rvalid;             /* IO-portregion has been requested */
	int leased;             /* Flag: This Adapter is connected  */
	/*       to a leased line           */
	unsigned short flags;   /* Statusflags                      */
	int doubleS0;           /* Flag: ICN4B                      */
	int secondhalf;         /* Flag: Second half of a doubleS0  */
	int fw_rev;             /* Firmware revision loaded         */
	int ptype;              /* Protocol type (1TR6 or Euro)     */
	struct timer_list st_timer;	/* Timer for Status-Polls           */
	struct timer_list rb_timer;	/* Timer for B-Channel-Polls        */
	u_char rcvbuf[ICN_BCH][4096];	/* B-Channel-Receive-Buffers        */
	int rcvidx[ICN_BCH];    /* Index for above buffers          */
	int l2_proto[ICN_BCH];  /* Current layer-2-protocol         */
	isdn_if interface;      /* Interface to upper layer         */
	int iptr;               /* Index to imsg-buffer             */
	char imsg[60];          /* Internal buf for status-parsing  */
	char msg_buf[2048];     /* Buffer for status-messages       */
	char *msg_buf_write;    /* Writepointer for statusbuffer    */
	char *msg_buf_read;     /* Readpointer for statusbuffer     */
	char *msg_buf_end;      /* Pointer to end of statusbuffer   */
	int sndcount[ICN_BCH];  /* Byte-counters for B-Ch.-send     */
	int xlen[ICN_BCH];      /* Byte-counters/Flags for sent-ACK */
	struct sk_buff *xskb[ICN_BCH];
	                        /* Current transmitted skb          */
	struct sk_buff_head
	 spqueue[ICN_BCH];      /* Sendqueue                        */
	char regname[35];       /* Name used for request_region     */
	u_char xmit_lock[ICN_BCH];	/* Semaphore for pollbchan_send()   */
} icn_card;

/*
 * Main driver data
 */
typedef struct icn_dev {
	icn_shmem *shmem;       /* Pointer to memory-mapped-buffers */
	int mvalid;             /* IO-shmem has been requested      */
	int channel;            /* Currently mapped channel         */
	struct icn_card *mcard; /* Currently mapped card            */
	int chanlock;           /* Semaphore for channel-mapping    */
	int firstload;          /* Flag: firmware never loaded      */
} icn_dev;

typedef icn_dev *icn_devptr;

#ifdef __KERNEL__

static icn_card *cards = (icn_card *) 0;
static u_char chan2bank[] =
{0, 4, 8, 12};                  /* for icn_map_channel() */

static icn_dev dev;

/* With modutils >= 1.1.67 Integers can be changed while loading a
 * module. For this reason define the Port-Base an Shmem-Base as
 * integers.
 */
static int portbase = ICN_BASEADDR;
static int membase = ICN_MEMADDR;
static char *icn_id = "\0";
static char *icn_id2 = "\0";

#ifdef MODULE
MODULE_AUTHOR("Fritz Elfert");
MODULE_PARM(portbase, "i");
MODULE_PARM_DESC(portbase, "Port address of first card");
MODULE_PARM(membase, "i");
MODULE_PARM_DESC(membase, "Shared memory address of all cards");
MODULE_PARM(icn_id, "s");
MODULE_PARM_DESC(icn_id, "ID-String of first card");
MODULE_PARM(icn_id2, "s");
MODULE_PARM_DESC(icn_id2, "ID-String of first card, second S0 (4B only)");
#endif

#endif                          /* __KERNEL__ */

/* Utility-Macros */

/* Macros for accessing ports */
#define ICN_CFG    (card->port)
#define ICN_MAPRAM (card->port+1)
#define ICN_RUN    (card->port+2)
#define ICN_BANK   (card->port+3)

/* Return true, if there is a free transmit-buffer */
#define sbfree (((readb(&dev.shmem->data_control.scns)+1) & 0xf) != \
		readb(&dev.shmem->data_control.scnr))

/* Switch to next transmit-buffer */
#define sbnext (writeb((readb(&dev.shmem->data_control.scns)+1) & 0xf, \
		       &dev.shmem->data_control.scns))

/* Shortcuts for transmit-buffer-access */
#define sbuf_n dev.shmem->data_control.scns
#define sbuf_d dev.shmem->data_buffers.send_buf[readb(&sbuf_n)].data
#define sbuf_l dev.shmem->data_buffers.send_buf[readb(&sbuf_n)].length
#define sbuf_f dev.shmem->data_buffers.send_buf[readb(&sbuf_n)].endflag

/* Return true, if there is receive-data is available */
#define rbavl  (readb(&dev.shmem->data_control.ecnr) != \
		readb(&dev.shmem->data_control.ecns))

/* Switch to next receive-buffer */
#define rbnext (writeb((readb(&dev.shmem->data_control.ecnr)+1) & 0xf, \
		       &dev.shmem->data_control.ecnr))

/* Shortcuts for receive-buffer-access */
#define rbuf_n dev.shmem->data_control.ecnr
#define rbuf_d dev.shmem->data_buffers.receive_buf[readb(&rbuf_n)].data
#define rbuf_l dev.shmem->data_buffers.receive_buf[readb(&rbuf_n)].length
#define rbuf_f dev.shmem->data_buffers.receive_buf[readb(&rbuf_n)].endflag

/* Shortcuts for command-buffer-access */
#define cmd_o (dev.shmem->comm_control.pcio_o)
#define cmd_i (dev.shmem->comm_control.pcio_i)

/* Return free space in command-buffer */
#define cmd_free ((readb(&cmd_i)>=readb(&cmd_o))? \
		  0x100-readb(&cmd_i)+readb(&cmd_o): \
		  readb(&cmd_o)-readb(&cmd_i))

/* Shortcuts for message-buffer-access */
#define msg_o (dev.shmem->comm_control.iopc_o)
#define msg_i (dev.shmem->comm_control.iopc_i)

/* Return length of Message, if avail. */
#define msg_avail ((readb(&msg_o)>readb(&msg_i))? \
		   0x100-readb(&msg_o)+readb(&msg_i): \
		   readb(&msg_i)-readb(&msg_o))

#define CID (card->interface.id)

#define MIN(a,b) ((a<b)?a:b)
#define MAX(a,b) ((a>b)?a:b)

/* Hopefully, a separate resource-registration-scheme for shared-memory
 * will be introduced into the kernel. Until then, we use the normal
 * routines, designed for port-registration.
 */
#define check_shmem   check_region
#define release_shmem release_region
#define request_shmem request_region

#endif                          /* defined(__KERNEL__) || defined(__DEBUGVAR__) */
#endif                          /* icn_h */
