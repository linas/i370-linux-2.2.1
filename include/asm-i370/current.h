#ifndef _I370_CURRENT_H
#define _I370_CURRENT_H

/* XXX this is probably all wrong */
/*
 * We keep `current' in r2 for speed.
 */
register struct task_struct *current asm ("r2");

#endif /* !(_I370_CURRENT_H) */
