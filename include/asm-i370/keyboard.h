/*
 * Keybaord defs XXX stubbed out.  Need to put this into 3270.h
 */

#ifndef __ASMI370_KEYBOARD_H
#define __ASMI370_KEYBOARD_H

#ifdef __KERNEL__

#include <linux/config.h>

/* XXX KEYBOARD_IRQ needs to be #defined so that pc_keyb.c will compile. 
 * But we shouldn't be compiling pc_keyb.c at all. So that needs fixing.
 */
#define KEYBOARD_IRQ			1
// #define DISABLE_KBD_DURING_INTERRUPTS	0
#define INIT_KBD

#if 0
extern int pckbd_setkeycode(unsigned int scancode, unsigned int keycode);
extern int pckbd_getkeycode(unsigned int scancode);
extern int pckbd_pretranslate(unsigned char scancode, char raw_mode);
extern int pckbd_translate(unsigned char scancode, unsigned char *keycode,
			   char raw_mode);
extern char pckbd_unexpected_up(unsigned char keycode);
extern void pckbd_init_hw(void);
#endif

static inline int kbd_setkeycode(unsigned int scancode, unsigned int keycode)
{
	// return pckbd_setkeycode(scancode,keycode);
	return 0;
}

static inline int kbd_getkeycode(unsigned int x)
{
	// return pckbd_getkeycode(x);
	return 0;
}

static inline int kbd_pretranslate(unsigned char x,char y)
{
	// return pckbd_pretranslate(x,y);
	return 0;
}

static inline int kbd_translate(unsigned char keycode, unsigned char *keycodep,
		     char raw_mode)
{
	// return pckbd_translate(keycode,keycodep,raw_mode);
	return 0;
}

static inline int kbd_unexpected_up(unsigned char keycode)
{
	// return pckbd_unexpected_up(keycode);
	return 0;
}

static inline void kbd_leds(unsigned char leds) { }

static inline void kbd_init_hw(void)
{
	// pckbd_init_hw();
	printk ("Keyboard hardware init\n");
}

#endif /* __KERNEL__ */

#endif /* __ASMI370_KEYBOARD_H */
