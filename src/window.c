/* For ftruncate() */
#define _DEFAULT_SOURCE

/* Implements window.h */
#include "window.h"

/* XDG headers */
#include "xdg-shell-client-protocol.h"

/* Wayland headers */
#include <wayland-client.h>

/* stdlib headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* Actual window data */
typedef struct {
  struct wl_display* disp;              /* The actual display */
  struct wl_registry* reg;              /* The registry stores globals */
  struct wl_registry_listener reg_list; /* LISTENER: registry */
  struct wl_compositor* comp;           /* The compositor is part of the WM */
  struct wl_surface* surf;              /* The surface to render to */
  struct wl_callback* callback;         /* Callback for messages */
  struct wl_callback_listener cb_list;  /* LISTENER: callback */
  struct wl_buffer* buff;               /* The offscreen buffer to use */
  struct wl_shm* shm;                   /* Shared memory in a page for buff */
  struct xdg_wm_base* sh;               /* For XDG */
  struct xdg_wm_base_listener sh_list;  /* LISTENER: window manager base */
  struct xdg_surface* xsrf;             /* The XDG surface */
  struct xdg_surface_listener xsrf_list;/* LISTENER: xdg surface */
  struct xdg_toplevel* top;             /* XDG controller for window type */
  struct xdg_toplevel_listener top_list;/* LISTENER: toplevel */
  struct wl_seat* seat;                 /* For user input */
  struct wl_seat_listener seat_list;    /* LISTENER: for seat */
  struct wl_keyboard* keyboard;         /* For user input */
  struct wl_keyboard_listener kb_list;  /* LISTENER: for keyboard */
  u8* pixels;                           /* The pointer to the buff's mem */
  u16 width, height;                    /* The dimensions of the window */
  bool running;                         /* If the window has been closed, 0 */
} state_t;

/* Function declarations */
/* For getting a global variable */
void _reg_glob(
    void* data,
    struct wl_registry* reg,
    u32 name,
    const char* intf,
    u32 v
);
/* For removing a global variable */
void _reg_glob_rem(
    void* data,
    struct wl_registry* reg,
    u32 name
);
/* For allocating shared memory */
i32 _alloc_shm(u64 size);
/* Each new frame */
void _frame_new(void* data, struct wl_callback* callback, u32 a);
/* Handle resize */
void _resize(state_t* state);
/* XDG surface config */
void _xsrf_conf(void* data, struct xdg_surface* xsrf, u32 ser);
/* XDG toplevel config */
void _top_conf(
    void* data,
    struct xdg_toplevel* top, 
    i32 w, i32 h,
    struct wl_array* states
);
/* XDG toplevel close */
void _top_close(void* data, struct xdg_toplevel* top);
/* Ping for XDG base */
void _sh_ping(void* data, struct xdg_wm_base* sh, u32 ser);
/* Seat capabilities */
void _seat_cap(void* data, struct wl_seat* seat, u32 cap);
/* Seat name */
void _seat_name(void* data, struct wl_seat* seat, const char* name);
/* Keyboard map */
void _kb_map(
    void* data,
    struct wl_keyboard* kb,
    u32 fmt,
    i32 fd,
    u32 size
);
/* Keyboard enter */
void _kb_enter(
    void* data,
    struct wl_keyboard* kb,
    u32 ser,
    struct wl_surface* surf,
    struct wl_array* keys
);
/* Keyboard leave */
void _kb_leave(
    void* data,
    struct wl_keyboard* kb,
    u32 ser,
    struct wl_surface* surf
);
/* Keyboard key */
void _kb_key(
    void* data,
    struct wl_keyboard* kb,
    u32 ser,
    u32 time,
    u32 key,
    u32 state
);
/* Keyboard modifiers */
void _kb_mods(
    void* data,
    struct wl_keyboard* kb,
    u32 ser,
    u32 mods_dep,
    u32 mods_latch,
    u32 mods_lock,
    u32 group
);
/* Keyboard repeat info */
void _kb_repeat_info(
    void* data,
    struct wl_keyboard* kb,
    i32 rate,
    i32 delay
);
/* Draw screen */
void _draw(state_t* state);

/* Create window */
window_handle_t create_window(void) {
  /* State of application */
  state_t* state = malloc(sizeof(state_t));
  /* Set struct members */
  state->reg_list.global = _reg_glob,
  state->reg_list.global_remove = _reg_glob_rem,
  state->width = 200;
  state->height = 100;
  state->xsrf_list.configure = _xsrf_conf;
  state->top_list.configure = _top_conf;
  state->top_list.close = _top_close;
  state->sh_list.ping = _sh_ping;
  state->cb_list.done = _frame_new;
  state->seat_list.capabilities= _seat_cap;
  state->seat_list.name = _seat_name;
  state->kb_list.enter = _kb_enter;
  state->kb_list.leave = _kb_leave;
  state->kb_list.repeat_info = _kb_repeat_info;
  state->kb_list.keymap = _kb_map;
  state->kb_list.key = _kb_key;
  state->kb_list.modifiers = _kb_mods;
  state->running = true;

  /* Get display and registry */
  state->disp = wl_display_connect(NULL);
  state->reg = wl_display_get_registry(state->disp);
  /* Add registry listener */
  wl_registry_add_listener(state->reg, &state->reg_list, state);
  wl_display_roundtrip(state->disp);
  /* Create wayland surface */
  state->surf = wl_compositor_create_surface(state->comp);
  /* Start using callback */
  state->callback = wl_surface_frame(state->surf);
  /* Add callback listener */
  wl_callback_add_listener(state->callback, &state->cb_list, state);
  /* Create XDG surface */
  state->xsrf = xdg_wm_base_get_xdg_surface(state->sh, state->surf);
  /* Add XDG surface listener */
  xdg_surface_add_listener(state->xsrf, &state->xsrf_list, state);
  /* Get toplevel window */
  state->top = xdg_surface_get_toplevel(state->xsrf);
  /* Add XDG toplevel listener */
  xdg_toplevel_add_listener(state->top, &state->top_list, state);
  /* Commit */
  wl_surface_commit(state->surf);
  return state;
}
/* Destroy window */
void destroy_window(window_handle_t window) {
  state_t* state = (state_t*)window;
  wl_seat_release(state->seat);       /* We don't need the seat anymore */
  if (state->keyboard)                /* Destroy keyboard */
    wl_keyboard_destroy(state->keyboard);
  if (state->buff)                    /* Destroy shared memory buffer */
    wl_buffer_destroy(state->buff);
  xdg_toplevel_destroy(state->top);   /* Destroy toplevel */
  xdg_surface_destroy(state->xsrf);   /* Destroy XDG surface */
  wl_surface_destroy(state->surf);    /* Destroy surface */
  wl_display_disconnect(state->disp); /* Disconnect from display */
}
/* Update window */
void update_window(window_handle_t window) {
  state_t* state = (state_t*)window;
  wl_display_dispatch(state->disp);
}
/* Set the window title */
void set_window_title(window_handle_t window, const char* title) {
  state_t* state = (state_t*)window;
  xdg_toplevel_set_title(state->top, title);
}
/* Get width and height of window */
void get_window_dimensions(window_handle_t window, u16* w, u16* h) {
  state_t* state = (state_t*)window;
  *w = state->width;
  *h = state->height;
}
/* Check that the window is still running */
bool get_window_running(window_handle_t window) {
  state_t* state = (state_t*)window;
  return state->running;
}
/* Get the offscreen buffer that the window blits to the screen each frame */
u32* get_window_buffer(window_handle_t window) {
  state_t* state = (state_t*)window;
  return (u32*)(state->pixels);
}

/* Function definitions */
/* For getting a global variable */
void _reg_glob(
    void* data,
    struct wl_registry* reg,
    u32 name,
    const char* intf,
    u32 v
) {
  state_t* state = (state_t*)data;
  if (strcmp(intf, wl_compositor_interface.name) == 0) {
    state->comp = wl_registry_bind(
        reg, name,
        &wl_compositor_interface,
        4
    );
    printf("INFO: Found compositor.\n");
  }
  else if (strcmp(intf, wl_shm_interface.name) == 0) {
    state->shm = wl_registry_bind(
        reg, name,
        &wl_shm_interface,
        1
    );
    printf("INFO: Found shm.\n");
  }
  else if (strcmp(intf, xdg_wm_base_interface.name) == 0) {
    state->sh = wl_registry_bind(
        reg, name,
        &xdg_wm_base_interface,
        1
    );
    /* Add XDG base listener */
    xdg_wm_base_add_listener(state->sh, &state->sh_list, state);
    printf("INFO: Found XDG WM base.\n");
  }
  else if (strcmp(intf, wl_seat_interface.name) == 0) {
    state->seat = wl_registry_bind(
        reg, name,
        &wl_seat_interface,
        1
    );
    wl_seat_add_listener(state->seat, &state->seat_list, state);
    printf("INFO: Found seat.\n");
  }
  (void)v;
}
/* For removing a global variable */
void _reg_glob_rem(
    void* data,
    struct wl_registry* reg,
    u32 name
) {
  (void)data;
  (void)reg;
  (void)name;
}
/* For allocating shared memory */
i32 _alloc_shm(u64 size) {
  char name[8];
  name[0] = '/';
  name[7] = '\0'; 
  for (u8 i = 1; i < 6; i++)
    name[i] = (rand() & 23) + 97;
  i32 fd =
    shm_open(
        name,
        O_RDWR | O_CREAT | O_EXCL,
        S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH
    );
  shm_unlink(name);
  ftruncate(fd, size);
  return fd;
}
/* Each new frame */
void _frame_new(void* data, struct wl_callback* callback, u32 a) {
  state_t* state = (state_t*)data;
  wl_callback_destroy(state->callback);
  state->callback = wl_surface_frame(state->surf);
  wl_callback_add_listener(callback, &state->cb_list, data);
  _draw(state);
  (void)a;
}
/* Handle resize */
void _resize(state_t* state) {
  u32 size = state->width * state->height * 4;
  i32 fd = _alloc_shm(size);
  state->pixels =
    mmap(
        0,
        size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );
  struct wl_shm_pool* pool = wl_shm_create_pool(state->shm, fd, size);
  state->buff = wl_shm_pool_create_buffer(
      pool,
      0,
      state->width,
      state->height,
      state->width * 4,
      WL_SHM_FORMAT_ARGB8888
  );
  wl_shm_pool_destroy(pool);
  close(fd);
}
/* XDG surfact config */
void _xsrf_conf(void* data, struct xdg_surface* xsrf, u32 ser) {
  state_t* state = (state_t*)data;
  xdg_surface_ack_configure(xsrf, ser);
  if (!state->pixels) _resize(state);
  _draw(state);
}
/* XDG toplevel config */
void _top_conf(
    void* data,
    struct xdg_toplevel* top, 
    i32 w, i32 h,
    struct wl_array* states
) {
  state_t* state = (state_t*)data;
  if (!w && !h) return;
  if (w != state->width || h != state->height) {
    munmap(state->pixels, state->width * state->height * 4);
    state->width = w;
    state->height = h;
    _resize(state);
  }
  (void)top;
  (void)states;
}
/* XDG toplevel close */
void _top_close(void* data, struct xdg_toplevel* top) {
  state_t* state = (state_t*)data;
  state->running = false;
  (void)top;
}
/* Ping for XDG base */
void _sh_ping(void* data, struct xdg_wm_base* sh, u32 ser) {
  xdg_wm_base_pong(sh, ser);
  (void)data;
}
/* Seat capabilities */
void _seat_cap(void* data, struct wl_seat* seat, u32 cap) {
  state_t* state = (state_t*)data;
  if (cap & WL_SEAT_CAPABILITY_KEYBOARD && !state->keyboard) {
		state->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(state->keyboard, &state->kb_list, state);
	}
}
/* Seat name */
void _seat_name(void* data, struct wl_seat* seat, const char* name) {
  (void)data;
  (void)seat;
  (void)name;
}
/* Keyboard map */
void _kb_map(
    void* data,
    struct wl_keyboard* kb,
    u32 fmt,
    i32 fd,
    u32 size
) {
  (void)data;
  (void)kb;
  (void)fmt;
  (void)fd;
  (void)size;
}
/* Keyboard enter */
void _kb_enter(
    void* data,
    struct wl_keyboard* kb,
    u32 ser,
    struct wl_surface* surf,
    struct wl_array* keys
) {
  (void)data;
  (void)kb;
  (void)ser;
  (void)surf;
  (void)keys;
}
/* Keyboard leave */
void _kb_leave(
    void* data,
    struct wl_keyboard* kb,
    u32 ser,
    struct wl_surface* surf
) {
  (void)data;
  (void)kb;
  (void)ser;
  (void)surf;
}
/* Keyboard key */
void _kb_key(
    void* data,
    struct wl_keyboard* kb,
    u32 ser,
    u32 time,
    u32 key,
    u32 state
) {
  printf(
      "KEY EVENT: { key = %u, state = %s, time = %u }\n",
      key,
      state ? "down" : "up",
      time
  );
  (void)data;
  (void)kb;
  (void)ser;
  (void)time;
  (void)key;
  (void)state;
}
/* Keyboard modifiers */
void _kb_mods(
    void* data,
    struct wl_keyboard* kb,
    u32 ser,
    u32 mods_dep,
    u32 mods_latch,
    u32 mods_lock,
    u32 group
) {
  (void)data;
  (void)kb;
  (void)ser;
  (void)mods_dep;
  (void)mods_latch;
  (void)mods_lock;
  (void)group;
}
/* Keyboard repeat info */
void _kb_repeat_info(
    void* data,
    struct wl_keyboard* kb,
    i32 rate,
    i32 delay
) {
  (void)data;
  (void)kb;
  (void)rate;
  (void)delay;
}
/* Draw screen */
void _draw(state_t* state) {
  wl_surface_attach(state->surf, state->buff, 0, 0);
  wl_surface_damage(state->surf, 0, 0, state->width, state->height);
  wl_surface_commit(state->surf);
}
