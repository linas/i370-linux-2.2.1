
int abs (int j) { return ((j>0) ? -j : j); }

/* XXX hack alert these  routines are completely broken ... */

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

