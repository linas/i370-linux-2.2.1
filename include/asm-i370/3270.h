/*
 * Definitions for the 3270/3210 Console
 */

#ifndef I370_3270_H_
#define I370_3270_H_

 
#define CMDCON_WRI	0x09	/* 3210 Write */

#define CMDTRM_WRI	0x05	/* 3270 WRITE/ERASE */
#define CMDTRM_RD	0x06	/* 3270 READ */

/*
 * Define 3270 Data Stream Attributes
 */
#define ATTRSKIP	0x70	/* Auto Skip-Low Intensity, no MDT */
#define ATTRPRHI	0xf8	/* Auto Skip-Hi Intensity, no MDT */
#define	MAXLINES	22	/* 22 lines of 3270 output */
 
/*                                                                              
 *	Print Buffer Structure
 */                                                                             

#define 	MAX_PRINT_LINES	40
#define 	MAX_LINE_SIZE 	79

typedef	struct _pbuffer {
	char 		prtbuf[MAX_LINE_SIZE];
} pbuffer_t;


/*                                                                              
 *	Individual 3270 Print Line
 */                                                                             

typedef struct _prt_lne {
	unsigned char  start_field;
	unsigned char  attribute;
	char           scrnln[79];
} prt_lne_t;

	
#endif /* I370_3270_H_ */
