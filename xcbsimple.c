/*
	Copyright (C) 2022-2025 <alpheratz99@protonmail.com>

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License version 2 as published by
	the Free Software Foundation.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
	more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc., 59
	Temple Place, Suite 330, Boston, MA 02111-1307 USA

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

#define _XOPEN_SOURCE 500

#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "fo.c"

#define UNUSED __attribute__((unused))
#define DEFAULT_MESSAGE "hi\nthis is a sample text\n\t\tfeel free to edit!\n"
#define RECT(x,y,w,h) ((const Rect) { x, y, w, h })
#define SPEED (8)

#define XCBSIMPLE_WM_NAME "xcbsimple"
#define XCBSIMPLE_WM_CLASS "xcbsimple\0xcbsimple\0"

typedef struct {
	uint32_t x, y;
	uint32_t width;
	uint32_t height;
} Rect;

static char *message;
static xcb_connection_t *conn;
static xcb_screen_t *screen;
static xcb_window_t window;
static xcb_gcontext_t gc;
static xcb_image_t *image;
static xcb_key_symbols_t *ksyms;
static uint32_t color;
static uint32_t width, height;
static int off_x = 20, off_y = 60;
static int mouse_x, mouse_y;
static uint32_t *px, pc;
static bool should_close;
static const char *filename;

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

static uint32_t
measure_word_width(const char *text)
{
	uint32_t word_width = 0;
	while (*text) {
		if (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r' || *text == '\0')
			break;
		word_width += 7;
		++text;
	}
	return word_width;
}

static void
render_text(const char *text, const Rect r, uint32_t color)
{
	uint32_t cx, cy, gx, gy, ux;
	unsigned char *glyph;
	const char *p;
	bool previous_was_blank = true;
	bool is_beggining_of_word;

	p = text;
	cx = r.x;
	cy = r.y;

	while (*p != '\0') {
		is_beggining_of_word = previous_was_blank;
		previous_was_blank = false;
		if (*p == '\n') {
			cx = r.x;
			cy += 10;
			previous_was_blank = true;
		} else if (*p == '\t') {
			cx += 7 * 4;
			previous_was_blank = true;
		} else if (*p == ' ') {
			cx += 7;
			previous_was_blank = true;
		} else {
			if (is_beggining_of_word) {
				uint32_t word_width = measure_word_width(p);
				if (mouse_x >= (int)cx && mouse_y >= (int)cy && mouse_y <= (int)cy + 7 && mouse_x < (int)cx + (int)word_width)
					for (ux = 0; ux < word_width; ++ux)
						if (cy + 9 < height && cy + 9 - r.y < r.height
								&& cx + ux < width && cx + ux - r.x <  r.width)
							px[(cy + 9) * width + cx + ux] = 0xff0000;

			}
			glyph = five_by_seven + *p*7;
			for (gy = 0; gy < 7; ++gy)
				for (gx = 0; gx < 5; ++gx)
					if (glyph[gy] & (1 << (4 - gx))
							&& cy + gy < height && cy + gy - r.y < r.height
							&& cx + gx <  width && cx + gx - r.x <  r.width)
						px[(cy + gy) * width + cx + gx] = color;
			cx += 7;
		}

		++p;
	}
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
		die("xcb_intern_atom failed with error code: %hhu",
				error->error_code);

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
						  XCB_EVENT_MASK_POINTER_MOTION |
			              XCB_EVENT_MASK_STRUCTURE_NOTIFY
		}}
	);

	xcb_create_gc(conn, gc, window, 0, NULL);

	image = xcb_image_create_native(
		conn, width, height, XCB_IMAGE_FORMAT_Z_PIXMAP, screen->root_depth,
		px, sizeof(uint32_t) * pc, (uint8_t *)(px)
	);

	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window, get_atom("_NET_WM_NAME"),
		get_atom("UTF8_STRING"), 8, sizeof(XCBSIMPLE_WM_NAME) - 1, XCBSIMPLE_WM_NAME
	);

	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_CLASS,
		XCB_ATOM_STRING, 8, sizeof(XCBSIMPLE_WM_CLASS) - 1,
		XCBSIMPLE_WM_CLASS
	);

	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window,
		get_atom("WM_PROTOCOLS"), XCB_ATOM_ATOM, 32, 1,
		(const xcb_atom_t []) { get_atom("WM_DELETE_WINDOW") }
	);

	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window,
		get_atom("_NET_WM_STATE"), XCB_ATOM_ATOM, 32, 1,
		(const xcb_atom_t []) { get_atom("_NET_WM_STATE_FULLSCREEN") }
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
prepare_render(void)
{
	size_t i;

	for (i = 0; i < pc; ++i)
		px[i] = color;

	char position[128] = {0};
	snprintf(&position[0], sizeof(position), "(%d, %d)", off_x, off_y);

	render_text(&position[0], RECT(off_x, off_y - 40, 500, 200), 0xff00f3);
	render_text(filename, RECT(off_x, off_y - 20, 500, 200), 0xffff00);
	render_text(message, RECT(off_x, off_y, width - off_x, height - off_y - 64), 0xffffff);

	render_text("h,j,k,l: move text around\nctrl+space: change background color", RECT(20, height-44, 500, 20), 0xaaaa00);
}

static void
set_color(uint32_t new_color)
{
	color = new_color;
}

static void
h_client_message(xcb_client_message_event_t *ev)
{
	/* check if the wm sent a delete window message */
	/* https://www.x.org/docs/ICCCM/icccm.pdf */
	if (ev->data.data32[0] == get_atom("WM_DELETE_WINDOW"))
		should_close = true;
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
			set_color(rand() % 0xffffff);
			prepare_render();
			xcb_image_put(conn, window, gc, image, 0, 0, 0);
			xcb_flush(conn);
		}
		break;
	case XKB_KEY_h:
	case XKB_KEY_j:
	case XKB_KEY_k:
	case XKB_KEY_l:
		off_x += ((key == XKB_KEY_h) * -1 + (key == XKB_KEY_l)) * SPEED;
		off_y += ((key == XKB_KEY_k) * -1 + (key == XKB_KEY_j)) * SPEED;
		prepare_render();
		xcb_image_put(conn, window, gc, image, 0, 0, 0);
		xcb_flush(conn);
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

	prepare_render();
	xcb_image_put(conn, window, gc, image, 0, 0, 0);
	xcb_flush(conn);
}

static void
h_motion_notify(xcb_motion_notify_event_t *ev)
{
	mouse_x = (int)ev->event_x;
	mouse_y = (int)ev->event_y;

	prepare_render();
	xcb_image_put(conn, window, gc, image, 0, 0, 0);
	xcb_flush(conn);
}

static void
h_mapping_notify(xcb_mapping_notify_event_t *ev)
{
	if (ev->count > 0)
		xcb_refresh_keyboard_mapping(ksyms, ev);
}

static char *
readfile_str(const char *path)
{
	FILE *fp;
	long file_size;
	char *raw_data;
	struct stat sb;

	if (stat(path, &sb) < 0)
		return strdup("Could not stat file");

	if (!S_ISREG(sb.st_mode))
		return strdup("Path does not belong to a file");

	if (NULL == (fp = fopen(path, "r")))
		return strdup("Cannot open file");

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	raw_data = malloc(file_size + 1);

	fread(raw_data, 1, file_size, fp);
	raw_data[file_size] = '\0';
	fclose(fp);

	return raw_data;
}

int
main(int argc, char **argv)
{
	xcb_generic_event_t *ev;

	message = argc > 1 ? readfile_str(argv[1]) : strdup(DEFAULT_MESSAGE);
	filename = argc > 1 ? argv[1] : "sample_file.txt";

	/* seed rand with the current process id */
	srand((unsigned int)(getpid()));

	create_window();
	set_color(0x290f53);
	prepare_render();

	while (!should_close && (ev = xcb_wait_for_event(conn))) {
		switch (ev->response_type & ~0x80) {
		case XCB_CLIENT_MESSAGE:     h_client_message((void *)(ev)); break;
		case XCB_EXPOSE:             h_expose((void *)(ev)); break;
		case XCB_KEY_PRESS:          h_key_press((void *)(ev)); break;
		case XCB_CONFIGURE_NOTIFY:   h_configure_notify((void *)(ev)); break;
		case XCB_MOTION_NOTIFY:      h_motion_notify((void *)(ev)); break;
		case XCB_MAPPING_NOTIFY:     h_mapping_notify((void *)(ev)); break;
		}

		free(ev);
	}

	destroy_window();
	free(message);

	return 0;
}
