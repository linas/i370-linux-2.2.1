        
/* system tracing */

#ifndef I370_TRACE_H
#define I370_TRACE_H
                                                                                

#define	MAX_TRACE_PAGES	4

/* 
 *	Define TRACE Page Structure
 */

typedef struct _trc_page  trc_page_t;

struct _trc_page {
	char		trc_bfr[4068];
	unsigned long	trc_cpuad;
	char		trc_todin[8];
	char		trc_todex[8];
	trc_page_t	*trc_next;
	unsigned long	trc_fill;
};
                                                                                
#endif /* I370_TRACE_H */
