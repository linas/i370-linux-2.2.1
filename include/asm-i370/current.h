#ifndef _I370_CURRENT_H
#define _I370_CURRENT_H

/*
 * Put `current' in a well-known, convenient register ... r2.
register struct task_struct *current asm ("r2");
 */
extern struct task_struct *current;

#endif /* !(_I370_CURRENT_H) */
