/*
 * linux/arch/i370/kernel/syscalls.c
 *
 * i370 version was blatantly stolen from PowerPC code
 * Derived from "arch/i386/kernel/i370_sys_i386.c"
 * Adapted from the i386 version by Gary Thomas
 * Modified by Cort Dougan (cort@cs.nmt.edu)
 * and Paul Mackerras (paulus@cs.anu.edu.au).
 * PowerPC version  Copyright (C) 1995-1996 Gary Thomas (gdt@linuxppc.org)
 *
 * This file contains various random system calls that
 * have a non-standard calling sequence on the Linux/390
 * platform.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/mman.h>
#include <linux/sys.h>
#include <linux/ipc.h>
#include <linux/utsname.h>

#include <asm/current.h>
#include <asm/uaccess.h>

void
check_bugs(void)
{
}

asmlinkage int i370_sys_execve(unsigned long fname, unsigned long argv,
                               unsigned long envp)
{
	int error;
	char * filename;
	struct pt_regs *regs;

	printk("i370_sys_execve: name = %s\n", (char *) fname);
	lock_kernel();
	regs = current->tss.regs;
	filename = getname((char *) fname);
	error = PTR_ERR(filename);
	if (IS_ERR(filename)) {
		printk("EXECVE Error: name = %s\n", (char *) fname);
	} else {
		error = do_execve(filename, (char **) argv, (char **) envp, regs);
		putname(filename);
	}
	unlock_kernel();
	printk("i370_sys_execve: return err=%d\n", error);
	return error;
}

asmlinkage int i370_sys_ptrace (void)
{
	printk("i370_sys_ptrace: unsupported\n");
	i370_halt();
	return 1;
}

asmlinkage int i370_sys_ioperm(unsigned long from, unsigned long num, int on)
{
	printk(KERN_ERR "i370_sys_ioperm()\n");
	return -EIO;
}

int i370_sys_iopl(int a1, int a2, int a3, int a4)
{
	lock_kernel();
	printk(KERN_ERR "i370_sys_iopl(%x, %x, %x, %x)!\n", a1, a2, a3, a4);
	unlock_kernel();
	return (-ENOSYS);
}

int i370_sys_vm86(int a1, int a2, int a3, int a4)
{
	lock_kernel();
	printk(KERN_ERR "i370_sys_vm86(%x, %x, %x, %x)!\n", a1, a2, a3, a4);
	unlock_kernel();
	return (-ENOSYS);
}

int i370_sys_modify_ldt(int a1, int a2, int a3, int a4)
{
	lock_kernel();
	printk(KERN_ERR "i370_sys_modify_ldt(%x, %x, %x, %x)!\n", a1, a2, a3, a4);
	unlock_kernel();
	return (-ENOSYS);
}

/*
 * i370_sys_ipc() is the de-multiplexer for the SysV IPC calls..
 *
 * This is really horribly ugly.
 */
asmlinkage int
i370_sys_ipc (uint call, int first, int second, int third, void *ptr, long fifth)
{
	int version, ret;

	lock_kernel();
	printk(KERN_ERR "i370_sys_ipc\n");
	version = call >> 16; /* hack for backward compatibility */
	call &= 0xffff;

	ret = -EINVAL;
#ifdef LATER
// XXX
	switch (call) {
	case SEMOP:
		ret = i370_sys_semop (first, (struct sembuf *)ptr, second);
		break;
	case SEMGET:
		ret = i370_sys_semget (first, second, third);
		break;
	case SEMCTL: {
		union semun fourth;

		if (!ptr)
			break;
		if ((ret = verify_area (VERIFY_READ, ptr, sizeof(long)))
		    || (ret = get_user(fourth.__pad, (void **)ptr)))
			break;
		ret = i370_sys_semctl (first, second, third, fourth);
		break;
		}
	case MSGSND:
		ret = i370_sys_msgsnd (first, (struct msgbuf *) ptr, second, third);
		break;
	case MSGRCV:
		switch (version) {
		case 0: {
			struct ipc_kludge tmp;

			if (!ptr)
				break;
			if ((ret = verify_area (VERIFY_READ, ptr, sizeof(tmp)))
			    || (ret = copy_from_user(&tmp,
						(struct ipc_kludge *) ptr,
						sizeof (tmp))))
				break;
			ret = i370_sys_msgrcv (first, tmp.msgp, second, tmp.msgtyp,
					  third);
			break;
			}
		default:
			ret = i370_sys_msgrcv (first, (struct msgbuf *) ptr,
					  second, fifth, third);
			break;
		}
		break;
	case MSGGET:
		ret = i370_sys_msgget ((key_t) first, second);
		break;
	case MSGCTL:
		ret = i370_sys_msgctl (first, second, (struct msqid_ds *) ptr);
		break;
	case SHMAT:
		switch (version) {
		default: {
			ulong raddr;

			if ((ret = verify_area(VERIFY_WRITE, (ulong*) third,
					       sizeof(ulong))))
				break;
			ret = i370_sys_shmat (first, (char *) ptr, second, &raddr);
			if (ret)
				break;
			ret = put_user (raddr, (ulong *) third);
			break;
			}
		case 1:	/* iBCS2 emulator entry point */
			if (!segment_eq(get_fs(), get_ds()))
				break;
			ret = i370_sys_shmat (first, (char *) ptr, second,
					 (ulong *) third);
			break;
		}
		break;
	case SHMDT:
		ret = i370_sys_shmdt ((char *)ptr);
		break;
	case SHMGET:
		ret = i370_sys_shmget (first, second, third);
		break;
	case SHMCTL:
		ret = i370_sys_shmctl (first, second, (struct shmid_ds *) ptr);
		break;
	}

#endif
	unlock_kernel();
	return ret;
}

/*
 * i370_sys_pipe() is the normal C calling standard for creating
 * a pipe. It's not the way unix traditionally does this, though.
 */
asmlinkage int i370_sys_pipe(int *fildes)
{
	int fd[2];
	int error;

	error = verify_area(VERIFY_WRITE, fildes, 8);
	if (error)
		return error;
	lock_kernel();
	error = do_pipe(fd);
	unlock_kernel();
	if (error)
		return error;
	if (__put_user(fd[0],0+fildes)
	    || __put_user(fd[1],1+fildes))
		return -EFAULT;	/* should we close the fds? */
	return 0;
}

asmlinkage unsigned long i370_sys_mmap(unsigned long addr, size_t len,
				  unsigned long prot, unsigned long flags,
				  unsigned long fd, off_t offset)
{
	struct file * file = NULL;
	int ret = -EBADF;

	lock_kernel();
	if (!(flags & MAP_ANONYMOUS)) {
		if (fd >= NR_OPEN || !(file = current->files->fd[fd]))
			goto out;
	}
	
	flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);
	ret = do_mmap(file, addr, len, prot, flags, offset);
out:
	unlock_kernel();
	return ret;
}


asmlinkage int i370_sys_pause(void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return -ERESTARTNOHAND;
}

asmlinkage int i370_sys_uname(struct old_utsname * name)
{
	if (name && !copy_to_user(name, &system_utsname, sizeof (*name)))
		return 0;
	return -EFAULT;
}

asmlinkage int i370_sys_olduname(struct oldold_utsname * name)
{
	int error;

	if (!name)
		return -EFAULT;
	if (!access_ok(VERIFY_WRITE,name,sizeof(struct oldold_utsname)))
		return -EFAULT;

	error = __copy_to_user(&name->sysname,&system_utsname.sysname,__OLD_UTS_LEN);
	error -= __put_user(0,name->sysname+__OLD_UTS_LEN);
	error -= __copy_to_user(&name->nodename,&system_utsname.nodename,__OLD_UTS_LEN);
	error -= __put_user(0,name->nodename+__OLD_UTS_LEN);
	error -= __copy_to_user(&name->release,&system_utsname.release,__OLD_UTS_LEN);
	error -= __put_user(0,name->release+__OLD_UTS_LEN);
	error -= __copy_to_user(&name->version,&system_utsname.version,__OLD_UTS_LEN);
	error -= __put_user(0,name->version+__OLD_UTS_LEN);
	error -= __copy_to_user(&name->machine,&system_utsname.machine,__OLD_UTS_LEN);
	error = __put_user(0,name->machine+__OLD_UTS_LEN);
	error = error ? -EFAULT : 0;

	return error;
}
