/*
 * keybaord defs XXX get rid of this its inappropriate
 */

#ifndef __ASMI370_KEYBOARD_H
#define __ASMI370_KEYBOARD_H

#ifdef __KERNEL__

#include <linux/config.h>

#define KEYBOARD_IRQ			1
#define DISABLE_KBD_DURING_INTERRUPTS	0
#define INIT_KBD

extern int pckbd_setkeycode(unsigned int scancode, unsigned int keycode);
extern int pckbd_getkeycode(unsigned int scancode);
extern int pckbd_pretranslate(unsigned char scancode, char raw_mode);
extern int pckbd_translate(unsigned char scancode, unsigned char *keycode,
			   char raw_mode);
extern char pckbd_unexpected_up(unsigned char keycode);
extern void pckbd_leds(unsigned char leds);
extern void pckbd_init_hw(void);

static inline int kbd_setkeycode(unsigned int scancode, unsigned int keycode)
{
	return pckbd_setkeycode(scancode,keycode);
}

static inline int kbd_getkeycode(unsigned int x)
{
	return pckbd_getkeycode(x);
}

static inline int kbd_pretranslate(unsigned char x,char y)
{
	return pckbd_pretranslate(x,y);
}

static inline int kbd_translate(unsigned char keycode, unsigned char *keycodep,
		     char raw_mode)
{
	return pckbd_translate(keycode,keycodep,raw_mode);
	
}

static inline int kbd_unexpected_up(unsigned char keycode)
{
	return pckbd_unexpected_up(keycode);
}

static inline void kbd_leds(unsigned char leds)
{
	pckbd_leds(leds);
}

static inline void kbd_init_hw(void)
{
	pckbd_init_hw();
}

#endif /* __KERNEL__ */

#endif /* __ASMI370_KEYBOARD_H */
