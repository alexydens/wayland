/* Project headers */
#include "utils.h"
#include "window.h"

/* stdlib headers */
#include <string.h>
#include <stdio.h>

/* Entry point */
int main(void) {
  /* Create window */
  window_handle_t window = create_window();
  set_window_title(window, "wayland window");
  /* Main loop */
  while (get_window_running(window)) {
    /* Update window */
    update_window(window);
    /* Clear screen */
    u16 w, h;
    u32* framebuffer = get_window_buffer(window);
    get_window_dimensions(window, &w, &h);
    memset(framebuffer, 0x00, w * h * 4);
    for (u32 i = 0; i < w * h; i++) {
      framebuffer[i] = 0xff00ff00;
    }
  }
  /* Destroy window */
  destroy_window(window);
  return 0;
}
