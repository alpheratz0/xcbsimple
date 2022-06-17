/*
	Copyright (C) 2022 <alpheratz99@protonmail.com>

	This program is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License version 2 as published by the
	Free Software Foundation.

	This program is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with
	this program; if not, write to the Free Software Foundation, Inc., 59 Temple
	Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "base/bitmap.h"
#include "util/debug.h"
#include "x11/keys.h"
#include "x11/window.h"

static window_t *window;

static void
key_press_callback(uint32_t key)
{
	switch (key) {
		case KEY_ESCAPE:
		case KEY_Q:
			window_loop_end(window);
			return;
		default:
			bitmap_clear(window->bmp, rand());
			window_force_redraw(window);
			break;
	}
}

int
main(void)
{
	/* unique seed between different runs */
	srand((unsigned int)(getuid()));

	window = window_create("xcbsimple", "xcbsimple");
	window_set_key_press_callback(window, key_press_callback);
	window_loop_start(window);
	window_free(window);

	return 0;
}
