/* $Id: param.h,v 1.1.1.1 1999/02/08 06:19:08 linas Exp $ */
#ifndef _ASMSPARC64_PARAM_H
#define _ASMSPARC64_PARAM_H

#ifndef HZ
#define HZ 100
#endif

#define EXEC_PAGESIZE	8192    /* Thanks for sun4's we carry baggage... */

#ifndef NGROUPS
#define NGROUPS		32
#endif

#ifndef NOGROUP
#define NOGROUP		(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */

#endif /* _ASMSPARC64_PARAM_H */
