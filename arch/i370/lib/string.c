
/*
 * quickie hacks that allow kernel to compile & function;
 * these need to be seriously performance tuned, as they
 * make up a significant amount of the cycles spent in the 
 * kernel.
 */

typedef int __kernel_size_t;

void *memset(void *s, int c, __kernel_size_t n)
{
   char *ss = (char *)s;
   int i=0;
   for (i=0; i<n; i++) {
      ss[i] = (char) c;
   }
   return s;
}

void *memchr(const void *s, int c, __kernel_size_t n)
{
   const char *ss = (const char *)s;
   int i=0;
   for (i=0; i<n; i++) {
      if (ss[i] == (char) c) return &(ss[i]);
   }
   return 0x0;
}

char* strncpy (char *dest, const char *src, __kernel_size_t n)
{
   int i=-1;
   n --;
   do {
      i++;
      dest[i] = src[i];
   } while (src[i] && (i<n));
   return dest;
}

/* ============================== END OF FILE ====================== */
