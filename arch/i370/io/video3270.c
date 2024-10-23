/*
 * video3270.c
 *
 * Rump for mapping 3270 functions to linux style 'video'.
 * This is meant to implement a full-screen driver where
 * text can be written anywhere, and the cursor can be
 * positioned anywhere.
 *
 * At this time, this is used only once, very early in boot.
 * Its used to print the message "Console: mono vid3270 80x24"
 * and then it's never used again. I guess that's because all
 * later messages go to the 3270/3210/3215 console. So I guess
 * we can leave this "unfinished", forever? Or should we swap
 * things around?
 *
 * --linas Oct 1999
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/selection.h>
#include <linux/console.h>
#include <linux/console_struct.h>
#include <linux/vt_kern.h>

/* static unsigned long vid3270_uni_pagedir[2] = {0,0}; */

/* ================================================================ */

__initfunc(static const char *vid3270_startup(void))
{
	return "vid3270";
}

/* ================================================================ */

static void
vid3270_init(struct vc_data *conp, int init)
{
	conp->vc_can_do_color = 0;
	if (init) {
		conp->vc_cols = 80;
		conp->vc_rows = 24;
	}
}

/* ================================================================ */

static void
vid3270_deinit(struct vc_data *conp)
{
}

/* ================================================================ */

static int
vid3270_switch(struct vc_data *conp)
{
	return 1;
}

/* ================================================================ */

static void
vid3270_clear(struct vc_data *conp, int sy, int sx, int height, int width)
{
}

/* ================================================================ */

static void
vid3270_bmove(struct vc_data *conp, int sy, int sx, int dy, int dx,
	      int height, int width)
{
}

/* ================================================================ */

static void
vid3270_putcs(struct vc_data *conp, const unsigned short *s,
              int count, int y, int x)
{
#define LINEBU 128
	char buf[LINEBU];
	int i;

	if (80 == count) {  /* quick hack don't print blank lines */
		for(i=0; i<count; i++) if (0x20 != (0xff & s[i])) goto prt;
		return;
	}
prt:

	// printk("vid3270_putcs %d chars at (%d,%d): ", count, y, x);

	if (LINEBU <= count)
	{
		printk("vid3270_putcs unexpectedly large put character string\n");
		for(i=0; i<count; i++)
			printk ("%c", 0xff & s[i]);
		printk ("\n");
	}
	else
	{
		for (i=0; i<count; i++)
			buf[i] = 0xff & s[i];
		buf[i] = '\n';
		buf[i+1] = 0;
		printk("%s", buf);
	}
}

/* ================================================================ */
/* put one character */

static void
vid3270_putc(struct vc_data *conp, int c, int y, int x)
{
	unsigned short s = c;
	vid3270_putcs(conp, &s, 1, y, x);
}

/* ================================================================ */

static void
vid3270_cursor(struct vc_data *conp, int mode)
{
	switch (mode) {
	case CM_ERASE:
	case CM_MOVE:
	case CM_DRAW:
		break;
	}
}

/* ================================================================ */

static int
vid3270_font_op(struct vc_data *conp, struct console_font_op *op)
{
	return -ENOSYS;
}

/* ================================================================ */

static int
vid3270_blank(struct vc_data *conp, int blank)
{
	if (blank) {
		/* do stuff here to blank the screen */
		return 0;
	} else {
		/* Let console.c redraw */
		return 1;
	}
}

/* ================================================================ */

static int
vid3270_scroll(struct vc_data *conp, int t, int b, int dir, int count)
{
	if (console_blanked) return 0;
	switch (dir) {
	case SM_UP:
	case SM_DOWN:
		break;
	}

	return 0;
}

/* ================================================================ */

static int
vid3270_set_palette(struct vc_data *c, unsigned char *table)
{
	return 0;
}

static int
vid3270_scrolldelta(struct vc_data *conp, int lines)
{
	return 0;
}

/* ================================================================ */
/* function table */

struct consw video3270_con = {
	con_startup:		vid3270_startup,
	con_init:		vid3270_init,
	con_deinit:		vid3270_deinit,
	con_clear:		vid3270_clear,
	con_putc:		vid3270_putc,
	con_putcs:		vid3270_putcs,
	con_cursor:		vid3270_cursor,
	con_scroll:		vid3270_scroll,
	con_bmove:		vid3270_bmove,
	con_switch:		vid3270_switch,
	con_blank:		vid3270_blank,
	con_font_op:		vid3270_font_op,
	con_set_palette:	vid3270_set_palette,
	con_scrolldelta:	vid3270_scrolldelta,
	con_set_origin:		NULL,
	con_save_screen:	NULL,
	con_build_attr:		NULL,
	con_invert_region:	NULL,
};


/* ================================================================ */
/* takover function */
/* XXX Note that its not really needed, as we already grabbed the console
 * in setup.c
 */

__initfunc(void video3270_init(void))
{
}

/* ================================================================ */
