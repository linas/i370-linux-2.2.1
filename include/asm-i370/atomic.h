/*
 * ESA/390 atomic operations
 * Copyright (c) 1999 Linas Vepstas
 */

#ifndef _ASM_I370_ATOMIC_H_ 
#define _ASM_I370_ATOMIC_H_

/* 
 * We use a funky typdef struct for this atomic type in order
 * to let the compiler catch any accidental assignemnts, increments, etc.
 */
#ifdef __SMP__
typedef struct { volatile int counter; } atomic_t;
#else
typedef struct { int counter; } atomic_t;
#endif

#define ATOMIC_INIT(i)	{ (i) }

#define atomic_read(v)		((v)->counter)

extern __inline__ void atomic_set(atomic_t *v, int setval)
{
	int oldval, newval;

	__asm__ __volatile__("
1:	L	%0,%2;
	L	%1,%2;
	AR	%1,%3;
	CS	%0,%1,%2;
	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*v)
	: "r" (setval)
	: "memory");
}

extern __inline__ int atomic_add_return(int inc, atomic_t *v)
{
	int oldval, newval;

	__asm__ __volatile__("
1:	L	%0,%2;
	L	%1,%2;
	AR	%1,%3;
	CS	%0,%1,%2;
	BNE	1b"
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

	__asm__ __volatile__("
1:	L	%0,%2;
	L	%1,%2;
	SR	%1,%3;
	CS	%0,%1,%2;
	BNE	1b"
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

	__asm__ __volatile__("
1:	L	%0,%2;
	L	%1,%2;
	SR	%1,%3;
	CS	%0,%1,%2;
	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*v)
	: "r" (inc)
	: "memory");

	return 0 == newval;
}

extern __inline__ int atomic_inc_return(atomic_t *v)
{
	int oldval, newval;

	__asm__ __volatile__("
1:	L	%0,%2;
	L	%1,%2;
	LA	%1,1(,%1);
	CS	%0,%1,%2;
	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*v)
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

	__asm__ __volatile__("
1:	L	%0,%2;
	L	%1,%2;
	SR	%1,%3;
	CS	%0,%1,%2;
	BNE	1b"
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

	__asm__ __volatile__("
1:	L	%0,%2;
	L	%1,%2;
	SR	%1,%3;
	CS	%0,%1,%2;
	BNE	1b"
	: "+r" (oldval), "+r" (newval), "+m" (*v)
	: "r" (inc)
	: "memory");

	return 0 == newval;
}

#endif /* _ASM_I370_ATOMIC_H_ */
