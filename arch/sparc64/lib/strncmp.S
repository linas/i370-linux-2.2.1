/* $Id: strncmp.S,v 1.1.1.1 1999/02/08 06:21:36 linas Exp $
 * Sparc64 optimized strncmp code.
 *
 * Copyright (C) 1997 Jakub Jelinek (jj@sunsite.mff.cuni.cz)
 */

#include <asm/asi.h>

	.text
	.align 4
	.global __strncmp, strncmp
__strncmp:
strncmp:
	brlez,pn %o2, 3f
	 lduba	[%o0] (ASI_PNF), %o3
1:
	add	%o0, 1, %o0
	ldub	[%o1], %o4
	brz,pn	%o3, 2f
	 add	%o1, 1, %o1
	cmp	%o3, %o4
	bne,pn	%icc, 2f
	 subcc	%o2, 1, %o2
	bne,a,pt %xcc, 1b
	 ldub	[%o0], %o3
2:
	retl
	 sub	%o3, %o4, %o0
3:
	retl
	 clr	%o0
