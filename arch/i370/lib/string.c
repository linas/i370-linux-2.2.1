

int strlen (const char *head) {
   char * tail = head;
   while (*tail) tail ++;
   return (tail - head);
}
