
/* video3270.c 
 *
 * Rump for mapping 3270 functions to linux style 'video'.
 * This is meant to implement a full-screen driver where 
 * text can be written anywhere, and the cursor can be 
 * positioned anywhere.
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


static unsigned long vid3270_uni_pagedir[2] = {0,0};

/* ================================================================ */

__initfunc(static const char *vid3270_startup(void))
{ 
	return "vid3270";
}

/* ================================================================ */

static void
vid3270_init(struct vc_data *conp, int init)
{
	unsigned long p;

	conp->vc_can_do_color = 0;
	if (init) {
		conp->vc_cols = 80;
		conp->vc_rows = 24;
	}

	/* beats the heck out of me wht this code does, but
	 * both the VGA and the promcon do this.  If we don't
	 * do this, cuhhh-rash. */
	p = *conp->vc_uni_pagedir_loc;
	if (conp->vc_uni_pagedir_loc == &conp->vc_uni_pagedir ||
	    !--conp->vc_uni_pagedir_loc[1])
		con_free_unimap(conp->vc_num);
	conp->vc_uni_pagedir_loc = vid3270_uni_pagedir;
	vid3270_uni_pagedir[1]++;
	if (!vid3270_uni_pagedir[0] && p) {
		con_set_default_unimap(conp->vc_num);
	}
}

/* ================================================================ */

static void
vid3270_deinit(struct vc_data *conp)
{
	if (!--vid3270_uni_pagedir[1]) {
		con_free_unimap(conp->vc_num);
	}
	conp->vc_uni_pagedir_loc = &conp->vc_uni_pagedir;
	con_set_default_unimap(conp->vc_num);
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
	int i;
	if (80 == count) {  /* quick hack don't print blank lines */
		for(i=0; i<count; i++) if (0x20 != 0xff & s[i]) goto prt;
		return;
	}
prt:
	printk("vid3270_putcs %d chars at (%d,%d): ", count, y, x);
	for(i=0; i<count; i++) {
		printk ("%c", 0xff & s[i]);
	} 
	printk ("\n");
}

/* ================================================================ */
/* put one character */

static void
vid3270_putc(struct vc_data *conp, int c, int y, int x)
{
	unsigned short s = c;
	if (console_blanked) return;
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
	}

	return 0;
}

/* ================================================================ */

static int vid3270_dummy(void)
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
	con_set_palette:	vid3270_dummy,
	con_scrolldelta:	vid3270_dummy,
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
	take_over_console(&video3270_con, 0, MAX_NR_CONSOLES-1, 1);
}

/* ================================================================ */