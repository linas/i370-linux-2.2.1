#ifndef _I370_IO_H
#define _I370_IO_H

// #define inb(port)		0
// #define outb(val, port)		
#define inb(port)		((int)(*((unsigned char *)(port))))
#define outb(val, port)		((*((unsigned char *)(port))) = (val))

#define sys_ioperm	i370_sys_ioperm

#endif
