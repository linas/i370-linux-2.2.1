
/*
 * Atomic add/sub/inc/dec operations
 * Implement with test and set  XXX
 * define as macros in asm/atomic.h  XXX hack alert
 */

#include <asm/atomic.h>


void atomic_add(int c, int *v) {}
void atomic_sub(int c, int *v) {}
void atomic_inc(int *v) {}
void atomic_dec(int *v) {}
int atomic_dec_and_test(int *v) {}
int atomic_inc_return(int *v) {}
int atomic_dec_return(int *v) {}
void atomic_clear_mask(atomic_t mask, atomic_t *addr) {}
void atomic_set_mask(atomic_t mask, atomic_t *addr) {}


