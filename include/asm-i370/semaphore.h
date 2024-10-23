#ifndef _I370_SEMAPHORE_H
#define _I370_SEMAPHORE_H

/*
 * SMP-safe and interrupt-safe semaphores..
 *
 * (C) Copyright 1996 Linus Torvalds
 * Adapted for PowerPC by Gary Thomas and Paul Mackerras
 */

#include <asm/atomic.h>

struct semaphore {
	atomic_t count;
	atomic_t waking;
	struct wait_queue * wait;
};

#define MUTEX ((struct semaphore) { ATOMIC_INIT(1), ATOMIC_INIT(0), NULL })
#define MUTEX_LOCKED ((struct semaphore) { ATOMIC_INIT(0), ATOMIC_INIT(0), NULL })

extern void __down(struct semaphore * sem);
extern int  __down_interruptible(struct semaphore * sem);
extern void __up(struct semaphore * sem);

#define sema_init(sem, val)	atomic_set(&((sem)->count), (val))

/*
 * For most semaphore operations, we can just use atomic_inc and
 * atomic_dec and everything just works. Except for one case: the
 * wake_one_more() and waking_non_zero() must be atomic w.r.t each
 * other.
 * -- wake_one_more() is easy; just atomic increment.
 * -- waking_non_zero() must decrement but only if the count is > 0
 *    That is, it needs to implement an atomic version of
 *
 *    inline int atomic_dec_if_positive(atomic_t *cnt) {
 *       int ret=0; if (*cnt > 0 ) { (*cnt)--; ret = 1; }  return ret; }
 *
 */

static inline void wake_one_more(struct semaphore * sem)
{
	atomic_inc(&sem->waking);
}

/* Decrement count, but only if positive. That is, an atomic version
 * of int ret=0; if (*cnt > 0 ) { (*cnt)--; ret = 1; }  return ret;
 * Since compare-n-swap doesn't do that, the implementation below
 * does this:
 *
 * retry:
 *    oldval= *cnt;
 *    newval = (*cnt)--;
 *    // At the (atomic) time that we looked, it wasn't positive. Bail.
 *    if (oldval <= 0) return 0;
 *    // If nothing changed, its safe to update.
 *    if (*cnt == oldval) { *cnt = newval; } else goto retry;
 *    return 1;
 *
 * I think this is correct.
 */
extern inline int atomic_dec_if_positive(atomic_t *cnt)
{
	int ret = 0;
	int inc = 1;
	int oldval, newval;

	__asm__ __volatile__(
"1:   L       %0,%2;"
"     LTR     %0,%0;"
"     BNH     2f;"
"     L       %1,%2;"
"     SR      %1,%4;"
"     CS      %0,%1,%2;"
"     BNE     1b;"
"     LA      %3,1(,0);"
"2:   ;"
   : "+r" (oldval), "+r" (newval), "+m" (*cnt), "+r" (ret)
   : "r" (inc)
   : "memory");

	return ret;
}

static inline int waking_non_zero(struct semaphore *sem,
                                  struct task_struct *tsk)
{
	return atomic_dec_if_positive(&sem->waking);
}

extern inline void down(struct semaphore * sem)
{
	if (atomic_dec_return(&sem->count) < 0)
		__down(sem);
}

extern inline int down_interruptible(struct semaphore * sem)
{
	int ret = 0;
	if (atomic_dec_return(&sem->count) < 0)
		ret = __down_interruptible(sem);
	return ret;
}

extern inline void up(struct semaphore * sem)
{
	if (atomic_inc_return(&sem->count) <= 0)
		__up(sem);
}	

#endif /* !(_I370_SEMAPHORE_H) */
