
#include <linux/init.h>

__initfunc(void mem_init(unsigned long start_mem, unsigned long end_mem))
{
}

/*
 * paging_init() sets up the page tables
 */
__initfunc(unsigned long paging_init(unsigned long start_mem, unsigned long
end_mem))
{
}


__initfunc(void free_initmem(void))
{
  printk ("do da free_initmem ding \n");
} 

