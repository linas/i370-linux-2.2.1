/*
 * The 3270 doesn't have keyboards.
 */
#ifndef __ASMI370_KEYBOARD_H
#define __ASMI370_KEYBOARD_H

#ifdef __KERNEL__
#include <linux/config.h>
#define INIT_KBD
#endif /* __KERNEL__ */

#endif /* __ASMI370_KEYBOARD_H */
