/*
 * ESA/390 atomic operations
 * Copyright (c) 1999 Linas Vepstas
 */

#ifndef _ASM_I370_ATOMIC_H_ 
#define _ASM_I370_ATOMIC_H_

/* 
 * We use a funky typdef struct for this atomic type in order
 * to let the compiler catch any accidental assignments, increments, etc.
 */
#ifdef __SMP__
typedef struct { volatile int counter; } atomic_t;
#else
typedef struct { int counter; } atomic_t;
#endif

#define ATOMIC_INIT(i)	{ (i) }

#define atomic_read(v)		((v)->counter)
#define atomic_set(v,i)		((v)->counter = (i))


/*
 * Atomic [test&set] exchange
 *
 *      void *xchg_u32(void *ptr, unsigned long val)
 * Changes the memory location '*ptr' to be val and returns
 * the previous value stored there.
 */
extern __inline__
void *xchg_u32(void *ptr, unsigned long newval) 
{
	unsigned long oldval;
	unsigned long *v = (unsigned long *) ptr;

	__asm__ __volatile__(
"1:	L	%0,%2;"
"	CS	%0,%1,%2;"
"	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*v)
	:
	: "memory");

	return (void *) oldval;
}


extern __inline__ int atomic_add_return(int inc, atomic_t *v)
{
	int oldval, newval;

	__asm__ __volatile__(
"1:	L	%0,%2;"
"	L	%1,%2;"
"	AR	%1,%3;"
"	CS	%0,%1,%2;"
"	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*v)
	: "r" (inc)
	: "memory");

	return newval;
}

extern __inline__ void atomic_add(int inc, atomic_t *v)
{
	atomic_add_return (inc,v);
}

extern __inline__ int atomic_sub_return(int inc, atomic_t *v)
{
	int oldval, newval;

	__asm__ __volatile__(
"1:	L	%0,%2;"
"	L	%1,%2;"
"	SR	%1,%3;"
"	CS	%0,%1,%2;"
"	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*v)
	: "r" (inc)
	: "memory");

	return newval;
}
extern __inline__ void atomic_sub(int inc, atomic_t *v)
{
	atomic_sub_return (inc,v);
}

extern __inline__ int atomic_sub_and_test(int inc, atomic_t *v)
{
	int oldval, newval;

	__asm__ __volatile__(
"1:	L	%0,%2;"
"	L	%1,%2;"
"	SR	%1,%3;"
"	CS	%0,%1,%2;"
"	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*v)
	: "r" (inc)
	: "memory");

	return 0 == newval;
}

extern __inline__ int atomic_inc_return(atomic_t *v)
{
	int oldval, newval;

	/* +r" allows registers r0-r15 while +a allows only r1-r15.
	 * This is needed to prevent LA from being used with r0 */
	__asm__ __volatile__(
"1:	L	%0,%2;"
"	L	%1,%2;"
"	LA	%1,1(,%1);"
"	CS	%0,%1,%2;"
"	BNE	1b"
	: "+r" (oldval), "+a" (newval), "+m" (*v)
	: 
	: "memory");

	return newval;
}
extern __inline__ void atomic_inc(atomic_t *v)
{
	atomic_inc_return (v);
}

extern __inline__ int atomic_dec_return(atomic_t *v)
{
	int oldval, newval;
	int inc = 1;

	__asm__ __volatile__(
"1:	L	%0,%2;"
"	L	%1,%2;"
"	SR	%1,%3;"
"	CS	%0,%1,%2;"
"	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*v)
	: "r" (inc)
	: "memory");

	return newval;
}
extern __inline__ void atomic_dec(atomic_t *v)
{
	atomic_dec_return (v);
}

extern __inline__ int atomic_dec_and_test(atomic_t *v)
{
	int oldval, newval;
	int inc = 1;

	__asm__ __volatile__(
"1:	L	%0,%2;"
"	L	%1,%2;"
"	SR	%1,%3;"
"	CS	%0,%1,%2;"
"	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*v)
	: "r" (inc)
	: "memory");

	return 0 == newval;
}

extern __inline__ void atomic_set_mask (unsigned long mask, unsigned long *addr)
{
	unsigned long oldval, newval;

	__asm__ __volatile__(
"1:	L	%0,%2;"
"	L	%1,%2;"
"	OR	%1,%3;"
"	CS	%0,%1,%2;"
"	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*addr)
	: "r" (mask)
	: "memory");
}

extern __inline__ void atomic_clear_mask (unsigned long mask, unsigned long *addr)
{
	unsigned long oldval, newval;

	mask = ~mask;	/* bit comopliment */

	__asm__ __volatile__(
"1:	L	%0,%2;"
"	L	%1,%2;"
"	NR	%1,%3;"
"	CS	%0,%1,%2;"
"	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*addr)
	: "r" (mask)
	: "memory");
}

/* implement traditional compare and swap api ... 
 * compare contents of "oldval" to contents of "memloc",
 * if equal, place "newval" at "memloc" and return zero,
 * else place contents of "memloc" in "oldval" and return non-zero.
*/
extern __inline__
unsigned long compare_and_swap (void *memloc, 
                                unsigned long *old, 
                                unsigned long newval)
{
	unsigned long *memptr = (unsigned long *) memloc;
	unsigned long rc = 0;

	/* +r" allows registers r0-r15 while +a allows only r1-r15.
	 * This is needed to prevent LA from being used with r0 */
	__asm__ __volatile__(
"	CS	%0,%1,%2;"
"	BZ	1f;"
"	LA	%3,1(,0);"
"	ST	%0,%4;"
"1:	;	"

	: "+r" (*old), "+r" (newval), "+m" (*memptr), "+a" (rc), "=m" (*old)
	: 
	: "memory");

	return rc;
}

#endif /* _ASM_I370_ATOMIC_H_ */
