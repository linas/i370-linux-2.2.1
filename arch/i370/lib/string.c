
/*
 * quickie hacks; could probably be made much more efficient 
 */

int strlen (const char *head) 
{
   char * tail = head;
   while (*tail) tail ++;
   return (tail - head);
}

int strcmp(const char *s1, const char *s2)
{
   int rc;
   int i=0;
   while (s1[i] && s2[i] && (s1[i] == s2[i])) i++;

   if (s1[i] != s2[i]) {
       return (s1[i] < s2[i]) ? 1 : -1 ;
   } 
   return 0;
}

void *memset(void *s, int c, size_t n)
{
   char *ss = (char *)s
   size_t i=0;
   for (i=0; i<n; i++) {
      ss[i] = (char) c;
   }
   return s;
}
