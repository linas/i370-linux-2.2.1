
/*
 * Atomic add/sub/inc/dec operations
 * Implement with test and set  XXX
 * define as macros in asm/atomic.h  XXX hack alert
 */

#include <asm/atomic.h>


void atomic_add(int a, atomic_t *v) {}
int  atomic_add_return(int a, atomic_t *v) {return 0;}
void atomic_sub(int a, atomic_t *v) {}
void atomic_inc(atomic_t *v) {}
int  atomic_inc_return(atomic_t *v) {return 0;}
void atomic_dec(atomic_t *v) {}
int  atomic_dec_return(atomic_t *v) {return 0;}
int  atomic_dec_and_test(atomic_t *v) {return 0;}

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

