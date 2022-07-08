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

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xproto.h>

#define UNUSED __attribute__((unused))

static xcb_connection_t *conn;
static xcb_window_t window;
static xcb_gcontext_t gc;
static xcb_image_t *image;
static uint32_t pixel_count;
static uint32_t *px;

static void
die(const char *err)
{
	fprintf(stderr, "xcbsimple: %s\n", err);
	exit(1);
}

static void
dief(const char *err, ...)
{
	va_list list;
	fputs("xcbsimple: ", stderr);
	va_start(list, err);
	vfprintf(stderr, err, list);
	va_end(list);
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

	error = NULL;
	cookie = xcb_intern_atom(conn, 1, strlen(name), name);
	reply = xcb_intern_atom_reply(conn, cookie, &error);

	if (NULL != error) {
		dief("xcb_intern_atom failed with error code: %d",
				(int)(error->error_code));
	}

	atom = reply->atom;
	free(reply);

	return atom;
}

static void
create_window(void)
{
	xcb_screen_t *screen;

	if (xcb_connection_has_error(conn = xcb_connect(NULL, NULL))) {
		die("can't open display");
	}

	if (NULL == (screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data)) {
		xcb_disconnect(conn);
		die("can't get default screen");
	}

	pixel_count = screen->width_in_pixels * screen->height_in_pixels;

	if (NULL == (px = malloc(pixel_count * sizeof(uint32_t)))) {
		die("error while calling malloc, no memory available");
	}

	window = xcb_generate_id(conn);
	gc = xcb_generate_id(conn);

	xcb_create_window(
		conn, XCB_COPY_FROM_PARENT, window, screen->root,
		0, 0, 800, 600, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual, XCB_CW_EVENT_MASK,
		(const uint32_t[]) {
			XCB_EVENT_MASK_EXPOSURE |
			XCB_EVENT_MASK_KEY_PRESS
		}
	);

	xcb_create_gc(conn, gc, window, 0, NULL);

	image = xcb_image_create_native(
		conn, screen->width_in_pixels, screen->height_in_pixels,
		XCB_IMAGE_FORMAT_Z_PIXMAP, screen->root_depth, px,
		sizeof(uint32_t)*pixel_count, (uint8_t *)(px)
	);

	/* set WM_NAME */
	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window,
		XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
		sizeof("xcbsimple") - 1, "xcbsimple"
	);

	/* set WM_CLASS */
	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window,
		XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8,
		sizeof("xcbsimple") * 2, "xcbsimple\0xcbsimple\0"
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
	xcb_image_destroy(image);
	xcb_disconnect(conn);
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
	size_t i;
	uint32_t color;

	for (color = rand(), i = 0; i < pixel_count; ++i) {
		px[i] = color;
	}

	xcb_image_put(conn, window, gc, image, 0, 0, 0);
}

static void
h_key_press(UNUSED xcb_key_press_event_t *ev)
{
	xcb_clear_area(conn, 1, window, 0, 0, 1, 1);
	xcb_flush(conn);
}

int
main(void)
{
	xcb_generic_event_t *ev;

	/* seed rand with the current process id */
	srand((unsigned int)(getpid()));

	create_window();

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
		}

		free(ev);
	}

	return 0;
}
