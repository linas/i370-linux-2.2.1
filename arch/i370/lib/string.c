
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
   int i=0;
   while (s1[i] && s2[i] && (s1[i] == s2[i])) i++;

   if (s1[i] != s2[i]) {
       return (s1[i] < s2[i]) ? 1 : -1 ;
   } 
   return 0;
}

void *memset(void *s, int c, int n)
{
   char *ss = (char *)s;
   int i=0;
   for (i=0; i<n; i++) {
      ss[i] = (char) c;
   }
   return s;
}

int
__copy_tofrom_user (char * to, char * from, int len) 
{

/* bogus */
}
