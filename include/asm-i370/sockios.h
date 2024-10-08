#ifndef _ASM_I370_SOCKIOS_H
#define _ASM_I370_SOCKIOS_H

#if 0 /* These are defined this way on Alpha - maybe later. */
/* Socket-level I/O control calls. */

#define FIOGETOWN	_IOR('f', 123, int)
#define FIOSETOWN 	_IOW('f', 124, int)

#define SIOCATMARK	_IOR('s', 7, int)
#define SIOCSPGRP	_IOW('s', 8, pid_t)
#define SIOCGPGRP	_IOR('s', 9, pid_t)

#define SIOCGSTAMP	0x8906		/* Get stamp - linux-specific */
#endif

#endif /* _ASM_I370_SOCKIOS_H */
