
/*
 * quickie hacks; could probably be made much more efficient 
 */

typedef int size_t;

int memcmp(const void *vs1, const void *vs2, size_t n) 
{
   int i=0;
   char * s1 = (char *) vs1;
   char * s2 = (char *) vs2;

   while ((s1[i] == s2[i]) && (i<n)) i++;

   if (i == n) return 0;
   if (s1[i] != s2[i]) {
       return (s1[i] < s2[i]) ? 1 : -1 ;
   } 
   return 0;
}


void* memcpy (void *dest, const void *src, size_t n)
{
   int i=0;
   char *d, *s;
   d = (char *)dest;
   s = (char *) src;
   // rewrite to inline MVCL
   for (i=0; i<n; i++) {
      d[i] = s[i];
   }
   return dest;
}

void *memset(void *s, int c, size_t n)
{
   char *ss = (char *)s;
   int i=0;
   for (i=0; i<n; i++) {
      ss[i] = (char) c;
   }
   return s;
}

int strcmp(const char *s1, const char *s2)
{
   int i=0;
   while (s1[i] && s2[i] && (s1[i] == s2[i])) i++;

   if (s1[i] != s2[i]) {
       return (s1[i] < s2[i]) ? 1 : -1 ;
   } 
   return 0;
}

char* strcpy (char *dest, const char *src)
{
   int i=-1;
   do {
      i++;
      dest[i] = src[i];
   } while (src[i]);
   return dest;
}

int strlen (const char *head) 
{
   char * tail = (char *) head;
   while (*tail) tail ++;
   return (tail - head);
}

char* strncpy (char *dest, const char *src, size_t n)
{
   int i=-1;
   n --;
   do {
      i++;
      dest[i] = src[i];
   } while (src[i] && (i<n));
   return dest;
}


int
__copy_tofrom_user (char * to, char * from, int len) 
{
/* bogus */
  return 0;
}
int __strncpy_from_user(char *dst, const char *src, long count)
{ return 0; }

 unsigned long __clear_user(void *addr, unsigned long size)
{ return 0; }
