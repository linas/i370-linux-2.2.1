#ifndef _I370_INIT_H
#define _I370_INIT_H

#if __GNUC__ > 2 || __GNUC_MINOR__ >= 90 /* egcs */
#define __init __attribute__ ((__section__ (".text.init")))
#define __initdata __attribute__ ((__section__ (".data.init")))
#define __initfunc(__arginit) \
	__arginit __init; \
	__arginit

#define __INIT		.section	".text.init",#alloc,#execinstr
#define __FINIT	.previous
#define __INITDATA	.section	".data.init",#alloc,#write

#define __cacheline_aligned __attribute__ \
			 ((__section__ (".data.cacheline_aligned")))

#else /* not egcs */

#define __init
#define __initdata
#define __initfunc(x) x
	
#define __INIT
#define __FINIT
#define __INITDATA
	

#define __cacheline_aligned
#endif /* egcs */
#endif
