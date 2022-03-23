#include <stdlib.h>
#include "bitmap.h"
#include "debug.h"
#include "keys.h"
#include "numdef.h"
#include "window.h"

static window_t *window;

static void
key_press_callback(u32 key) {
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
main(void) {
	window = window_create("xcbsimple");
	window_set_key_press_callback(window, key_press_callback);
	window_loop_start(window);
	window_free(window);
	return 0;
}
