
/*
 * Atomic add/sub/inc/dec operations
 * Implement with test and set  XXX
 * define as macros in asm/atomic.h  XXX hack alert
 */

#include <asm/atomic.h>

/* XXX hack alert these atomic routines are completely broken ... */

void atomic_add(int a, atomic_t *v)
{
   atomic_t t;
   t = *v;
   t.counter += a;
   *v = t;
}

int  atomic_add_return(int a, atomic_t *v) 
{
   atomic_t t;
   t = *v;
   t.counter += a;
   *v = t;
   return t.counter;
}

void atomic_sub(int a, atomic_t *v) 
{
   atomic_t t;
   t = *v;
   t.counter -= a;
   *v = t;
}

void atomic_inc(atomic_t *v)
{
   atomic_t t;
   t = *v;
   t.counter ++;
   *v = t;
}

int  atomic_inc_return(atomic_t *v) 
{
   atomic_t t;
   t = *v;
   t.counter ++;
   *v = t;
   return t.counter;
}

void atomic_dec(atomic_t *v) 
{
   atomic_t t;
   t = *v;
   t.counter --;
   *v = t;
}

int  atomic_dec_return(atomic_t *v) 
{
   atomic_t t;
   t = *v;
   t.counter --;
   *v = t;
   return t.counter;
}

int  atomic_dec_and_test(atomic_t *v) 
{
   atomic_t t;
   t = *v;
   t.counter --;
   *v = t;
   return 0 == t.counter;
}


void atomic_clear_mask(unsigned long mask, unsigned long *addr) {}
void atomic_set_mask(unsigned long mask, unsigned long *addr) {}


/*
 * Atomic [test&set] exchange
 *
 *      void *xchg_u32(void *ptr, unsigned long val)
 * Changes the memory location '*ptr' to be val and returns
 * the previous value stored there.
 */
void *xchg_u32(void *ptr, unsigned long val) { return ptr;}


int abs (int j) { return ((j>0) ? -j : j); }


/*
 * computes the checksum of a memory block at buff, length len,
 * and adds in "sum" (32-bit)
 *
 * returns a 32-bit number suitable for feeding into itself
 * or csum_tcpudp_magic
 *
 * this function must be called with even lengths, except
 * for the last fragment, which may be odd
 *
 * it's best to have buff aligned on a 32-bit boundary
 */
unsigned int csum_partial(const unsigned char * buff, int len,
                                 unsigned int sum) 
{ return 0; }

/*
 * Computes the checksum of a memory block at src, length len,
 * and adds in "sum" (32-bit), while copying the block to dst.
 * If an access exception occurs on src or dst, it stores -EFAULT
 * to *src_err or *dst_err respectively (if that pointer is not
 * NULL), and, for an error on src, zeroes the rest of dst.
 *
 * Like csum_partial, this must be called with even lengths,
 * * except for the last fragment.
 */
unsigned int csum_partial_copy_generic(const char *src, char *dst,
                                              int len, unsigned int sum,
                                              int *src_err, int *dst_err)
{ return 0; }

