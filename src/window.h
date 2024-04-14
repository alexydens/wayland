/* Include guard */
#ifndef WINDOW_H
#define WINDOW_H

/* Project headers */
#include "utils.h"

/* Window type - points to internal state */
typedef void* window_handle_t;

/* Create window */
extern window_handle_t create_window(void);
/* Destroy window */
extern void destroy_window(window_handle_t window);
/* Update window */
extern void update_window(window_handle_t window);
/* Set the window title */
extern void set_window_title(window_handle_t window, const char* title);
/* Get width and height of window */
extern void get_window_dimensions(window_handle_t window, u16* w, u16* h);
/* Check that the window is still running */
extern bool get_window_running(window_handle_t window);
/* Get the offscreen buffer that the window blits to the screen each frame */
extern u32* get_window_buffer(window_handle_t window);

#endif /* WINDOW_H */
