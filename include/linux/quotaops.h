/*
 * Definitions for diskquota-operations. When diskquota is configured these
 * macros expand to the right source-code.
 *
 * Author:  Marco van Wieringen <mvw@planets.elm.net>
 *
 * Version: $Id: quotaops.h,v 1.1.1.1 1999/02/08 06:18:48 linas Exp $
 *
 */
#ifndef _LINUX_QUOTAOPS_
#define _LINUX_QUOTAOPS_

#include <linux/config.h>

#if defined(CONFIG_QUOTA)

/*
 * declaration of quota_function calls in kernel.
 */
extern void dquot_initialize(struct inode *inode, short type);
extern void dquot_drop(struct inode *inode);
extern void invalidate_dquots(kdev_t dev, short type);
extern int  quota_off(kdev_t dev, short type);
extern int  sync_dquots(kdev_t dev, short type);

extern int  dquot_alloc_block(const struct inode *inode, unsigned long number,
                              uid_t initiator, char warn);
extern int  dquot_alloc_inode(const struct inode *inode, unsigned long number,
                              uid_t initiator);

extern void dquot_free_block(const struct inode *inode, unsigned long number);
extern void dquot_free_inode(const struct inode *inode, unsigned long number);

extern int  dquot_transfer(struct inode *inode, struct iattr *iattr,
                           char direction, uid_t initiator);

/*
 * Operations supported for diskquotas.
 */
extern __inline__ void DQUOT_INIT(struct inode *inode)
{
	if (inode->i_sb && inode->i_sb->dq_op)
		inode->i_sb->dq_op->initialize(inode, -1);
}

extern __inline__ void DQUOT_DROP(struct inode *inode)
{
	if (IS_QUOTAINIT(inode)) {
		if (inode->i_sb && inode->i_sb->dq_op)
			inode->i_sb->dq_op->drop(inode);
	}
}

extern __inline__ int DQUOT_PREALLOC_BLOCK(struct super_block *sb, const struct inode *inode, int nr)
{
	if (sb->dq_op) {
		if (sb->dq_op->alloc_block(inode, fs_to_dq_blocks(nr, sb->s_blocksize),
					   current->euid, 0) == NO_QUOTA)
			return 1;
	}
	return 0;
}

extern __inline__ int DQUOT_ALLOC_BLOCK(struct super_block *sb, const struct inode *inode, int nr)
{
	if (sb->dq_op) {
		if (sb->dq_op->alloc_block(inode, fs_to_dq_blocks(nr, sb->s_blocksize),
					   current->euid, 1) == NO_QUOTA)
			return 1;
	}
	return 0;
}

extern __inline__ int DQUOT_ALLOC_INODE(struct super_block *sb, struct inode *inode)
{
	if (sb->dq_op) {
		sb->dq_op->initialize (inode, -1);
		if (sb->dq_op->alloc_inode (inode, 1, current->euid))
			return 1;
	}
	inode->i_flags |= S_QUOTA;
	return 0;
}

extern __inline__ void DQUOT_FREE_BLOCK(struct super_block *sb, const struct inode *inode, int nr)
{
	if (sb->dq_op)
		sb->dq_op->free_block(inode, fs_to_dq_blocks(nr, sb->s_blocksize));
}

extern __inline__ void DQUOT_FREE_INODE(struct super_block *sb, struct inode *inode)
{
	if (sb->dq_op)
		sb->dq_op->free_inode(inode, 1);
}

extern __inline__ int DQUOT_TRANSFER(struct dentry *dentry, struct iattr *iattr)
{
	int error = -EDQUOT;

	if (dentry->d_inode->i_sb->dq_op) {
		if (IS_QUOTAINIT(dentry->d_inode) == 0)
			dentry->d_inode->i_sb->dq_op->initialize(dentry->d_inode, -1);
		if (dentry->d_inode->i_sb->dq_op->transfer(dentry->d_inode, iattr, 0, current->euid))
			goto out;
		error = notify_change(dentry, iattr);
		if (error)
			dentry->d_inode->i_sb->dq_op->transfer(dentry->d_inode, iattr, 1, current->euid);
	} else {
		error = notify_change(dentry, iattr);
	}
out:
	return error;
}

#define DQUOT_SYNC(dev)	sync_dquots(dev, -1)
#define DQUOT_OFF(dev)	quota_off(dev, -1)

#else

/*
 * NO-OP when quota not configured.
 */
#define DQUOT_INIT(inode)			do { } while(0)
#define DQUOT_DROP(inode)			do { } while(0)
#define DQUOT_PREALLOC_BLOCK(sb, inode, nr)	(0)
#define DQUOT_ALLOC_BLOCK(sb, inode, nr)	(0)
#define DQUOT_ALLOC_INODE(sb, inode)		(0)
#define DQUOT_FREE_BLOCK(sb, inode, nr)		do { } while(0)
#define DQUOT_FREE_INODE(sb, inode)		do { } while(0)
#define DQUOT_SYNC(dev)				do { } while(0)
#define DQUOT_OFF(dev)				do { } while(0)

/*
 * Special case expands to a simple notify_change.
 */
#define DQUOT_TRANSFER(dentry, iattr) notify_change(dentry, iattr)

#endif /* CONFIG_QUOTA */
#endif /* _LINUX_QUOTAOPS_ */
