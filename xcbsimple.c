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

	 _____________________________________
	( things are getting slow and slow... )
	 -------------------------------------
	    o                                  ___-------___
	     o                             _-~~             ~~-_
	      o                         _-~                    /~-_
	             /^\__/^\         /~  \                   /    \
	           /|  O|| O|        /      \_______________/        \
	          | |___||__|      /       /                \          \
	          |          \    /      /                    \          \
	          |   (_______) /______/                        \_________ \
	          |         / /         \                      /            \
	           \         \^\\         \                  /               \     /
	             \         ||           \______________/      _-_       //\__//
	               \       ||------_-~~-_ ------------- \ --/~   ~\    || __/
	                 ~-----||====/~     |==================|       |/~~~~~
	                  (_(__/  ./     /                    \_\      \.
	                         (_(___/                         \_____)_)

*/

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#define UNUSED __attribute__((unused))

static xcb_connection_t *conn;
static xcb_screen_t *screen;
static xcb_window_t window;
static xcb_gcontext_t gc;
static xcb_image_t *image;
static xcb_key_symbols_t *ksyms;
static uint32_t color;
static uint32_t width, height;
static uint32_t *px, pc;

static void
die(const char *fmt, ...)
{
	va_list args;

	fputs("xcbsimple: ", stderr);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
	exit(1);
}

static xcb_atom_t
get_atom(const char *name)
{
	xcb_atom_t atom;
	xcb_generic_error_t *error;
	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t *reply;

	cookie = xcb_intern_atom(conn, 0, strlen(name), name);
	reply = xcb_intern_atom_reply(conn, cookie, &error);

	if (NULL != error)
		die("xcb_intern_atom failed with error code: %d",
				(int)(error->error_code));

	atom = reply->atom;
	free(reply);

	return atom;
}

static void
create_window(void)
{
	if (xcb_connection_has_error(conn = xcb_connect(NULL, NULL)))
		die("can't open display");

	if (NULL == (screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data))
		die("can't get default screen");

	width = height = 600;
	pc = width * height;

	if (NULL == (px = malloc(sizeof(uint32_t) * pc)))
		die("error while calling malloc, no memory available");

	ksyms = xcb_key_symbols_alloc(conn);
	window = xcb_generate_id(conn);
	gc = xcb_generate_id(conn);

	xcb_create_window_aux(
		conn, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0,
		width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual, XCB_CW_EVENT_MASK,
		(const xcb_create_window_value_list_t []) {{
			.event_mask = XCB_EVENT_MASK_EXPOSURE |
			              XCB_EVENT_MASK_KEY_PRESS |
			              XCB_EVENT_MASK_STRUCTURE_NOTIFY
		}}
	);

	xcb_create_gc(conn, gc, window, 0, NULL);

	image = xcb_image_create_native(
		conn, width, height, XCB_IMAGE_FORMAT_Z_PIXMAP, screen->root_depth,
		px, sizeof(uint32_t) * pc, (uint8_t *)(px)
	);

	/* set WM_NAME */
	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
		XCB_ATOM_STRING, 8, sizeof("xcbsimple") - 1, "xcbsimple"
	);

	/* set WM_CLASS */
	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_CLASS,
		XCB_ATOM_STRING, 8, sizeof("xcbsimple") * 2, "xcbsimple\0xcbsimple\0"
	);

	/* add WM_DELETE_WINDOW to WM_PROTOCOLS */
	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window,
		get_atom("WM_PROTOCOLS"), XCB_ATOM_ATOM, 32, 1,
		(const xcb_atom_t[]) { get_atom("WM_DELETE_WINDOW") }
	);

	/* set FULLSCREEN */
	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window,
		get_atom("_NET_WM_STATE"), XCB_ATOM_ATOM, 32, 1,
		(const xcb_atom_t[]) { get_atom("_NET_WM_STATE_FULLSCREEN") }
	);

	xcb_map_window(conn, window);
	xcb_flush(conn);
}

static void
destroy_window(void)
{
	xcb_free_gc(conn, gc);
	xcb_key_symbols_free(ksyms);
	xcb_image_destroy(image);
	xcb_disconnect(conn);
}

static void
paint_solid_color(uint32_t color)
{
	size_t i;

	for (i = 0; i < pc; ++i)
		px[i] = color;
}

static void
h_client_message(xcb_client_message_event_t *ev)
{
	/* check if the wm sent a delete window message */
	/* https://www.x.org/docs/ICCCM/icccm.pdf */
	if (ev->data.data32[0] == get_atom("WM_DELETE_WINDOW")) {
		destroy_window();
		exit(0);
	}
}

static void
h_expose(UNUSED xcb_expose_event_t *ev)
{
	xcb_image_put(conn, window, gc, image, 0, 0, 0);
}

static void
h_key_press(xcb_key_press_event_t *ev)
{
	xcb_keysym_t key;

	key = xcb_key_symbols_get_keysym(ksyms, ev->detail, 0);

	switch (key) {
		case XKB_KEY_space:
			if (ev->state & XCB_MOD_MASK_CONTROL) {
				paint_solid_color((color = rand()));
				xcb_image_put(conn, window, gc, image, 0, 0, 0);
				xcb_flush(conn);
			}
			break;
	}
}

static void
h_configure_notify(xcb_configure_notify_event_t *ev)
{
	if (width == ev->width && height == ev->height)
		return;

	width = ev->width;
	height = ev->height;
	pc = width * height;

	xcb_image_destroy(image);
	px = malloc(sizeof(uint32_t) * pc);

	image = xcb_image_create_native(
		conn, width, height, XCB_IMAGE_FORMAT_Z_PIXMAP, screen->root_depth,
		px, sizeof(uint32_t) * pc, (uint8_t *)(px)
	);

	paint_solid_color(color);
	xcb_image_put(conn, window, gc, image, 0, 0, 0);
	xcb_flush(conn);
}

static void
h_mapping_notify(xcb_mapping_notify_event_t *ev)
{
	if (ev->count > 0)
		xcb_refresh_keyboard_mapping(ksyms, ev);
}

int
main(void)
{
	xcb_generic_event_t *ev;

	/* seed rand with the current process id */
	srand((unsigned int)(getpid()));

	create_window();
	paint_solid_color((color = rand()));

	while ((ev = xcb_wait_for_event(conn))) {
		switch (ev->response_type & ~0x80) {
			case XCB_CLIENT_MESSAGE:
				h_client_message((xcb_client_message_event_t *)(ev));
				break;
			case XCB_EXPOSE:
				h_expose((xcb_expose_event_t *)(ev));
				break;
			case XCB_KEY_PRESS:
				h_key_press((xcb_key_press_event_t *)(ev));
				break;
			case XCB_CONFIGURE_NOTIFY:
				h_configure_notify((xcb_configure_notify_event_t *)(ev));
				break;
			case XCB_MAPPING_NOTIFY:
				h_mapping_notify((xcb_mapping_notify_event_t *)(ev));
				break;
		}

		free(ev);
	}

	destroy_window();

	return 0;
}
