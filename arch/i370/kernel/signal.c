
/* XXX signal.c not implemented. Take a look at ppc/signal.h for ideas. */

#include <linux/kernel.h>
#include <asm-i370/asm.h>

void
i370_do_signal (void)
{
	printk ("called do_signal XXX not implemented \n");
	i370_halt();
}
