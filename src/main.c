/* For ftruncate() */
#define _DEFAULT_SOURCE

/* Project headers */
#include "utils.h"

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

/* Functions declarations */
/* For getting a global variable */
void reg_glob(
    void* data,
    struct wl_registry* reg,
    u32 name,
    const char* intf,
    u32 v
);
/* Each new frame */
void frame_new(void* data, struct wl_callback* callback, u32 a);
/* For removing a global variable */
void reg_glob_rem(
    void* data,
    struct wl_registry* reg,
    u32 name
);
/* For allocating shared memory */
i32 alloc_shm(u64 size);
/* Handle resize */
void resize(void);
/* XDG surface config */
void xsrf_conf(void* data, struct xdg_surface* xsrf, u32 ser);
/* XDG toplevel config */
void top_conf(
    void* data,
    struct xdg_toplevel* top, 
    i32 w, i32 h,
    struct wl_array* states
);
/* XDG toplevel close */
void top_close(void* data, struct xdg_toplevel* top);
/* Ping for XDG base */
void sh_ping(void* data, struct xdg_wm_base* sh, u32 ser);
/* Draw screen */
void draw(void);
/* Put one pixel to the screen */
void putpixel(u16 x, u16 y, u32 pixel);

/* Global state */
struct {
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
  u8* pixels;                            /* The pointer to the buff's mem */
  u16 width, height;                    /* The dimensions of the window */
} state;

/* Entry point */
int main(void) {
  /* Set struct members */
  state.reg_list.global = reg_glob,
  state.reg_list.global_remove = reg_glob_rem,
  state.width = 200;
  state.height = 100;
  state.xsrf_list.configure = xsrf_conf;
  state.top_list.configure = top_conf;
  state.top_list.close = top_close;
  state.sh_list.ping = sh_ping;
  state.cb_list.done = frame_new;

  /* Get display and registry */
  state.disp = wl_display_connect(NULL);
  state.reg = wl_display_get_registry(state.disp);
  /* Add registry listener */
  wl_registry_add_listener(state.reg, &state.reg_list, NULL);
  wl_display_roundtrip(state.disp);
  /* Create wayland surface */
  state.surf = wl_compositor_create_surface(state.comp);
  /* Start using callback */
  state.callback = wl_surface_frame(state.surf);
  /* Add callback listener */
  wl_callback_add_listener(state.callback, &state.cb_list, NULL);
  /* Create XDG surface */
  state.xsrf = xdg_wm_base_get_xdg_surface(state.sh, state.surf);
  /* Add XDG surface listener */
  xdg_surface_add_listener(state.xsrf, &state.xsrf_list, NULL);
  /* Get toplevel window */
  state.top = xdg_surface_get_toplevel(state.xsrf);
  /* Add XDG toplevel listener */
  xdg_toplevel_add_listener(state.top, &state.top_list, NULL);
  /* Change title... and commit! */
  xdg_toplevel_set_title(state.top, "wayland client");
  wl_surface_commit(state.surf);

  /* Main loop */
  while (wl_display_dispatch(state.disp));

  /* Terminate */
  if (state.buff)
    wl_buffer_destroy(state.buff);    /* Destroy shared memory buffer */
  xdg_toplevel_destroy(state.top);    /* Destroy toplevel */
  xdg_surface_destroy(state.xsrf);    /* Destroy XDG surface */
  wl_surface_destroy(state.surf);     /* Destroy surface */
  wl_display_disconnect(state.disp);  /* Disconnect from display */
  return 0;
}

/* Functions definitions */
/* For getting a global variable */
void reg_glob(
    void* data,
    struct wl_registry* reg,
    u32 name,
    const char* intf,
    u32 v
) {
  if (strcmp(intf, wl_compositor_interface.name) == 0)
    state.comp = wl_registry_bind(
        reg, name,
        &wl_compositor_interface,
        4
    );
  else if (strcmp(intf, wl_shm_interface.name) == 0)
    state.shm = wl_registry_bind(
        reg, name,
        &wl_shm_interface,
        1
    );
  else if (strcmp(intf, xdg_wm_base_interface.name) == 0) {
    state.sh = wl_registry_bind(
        reg, name,
        &xdg_wm_base_interface,
        1
    );
    /* Add XDG base listener */
    xdg_wm_base_add_listener(state.sh, &state.sh_list, NULL);
  }
  (void)data;
  (void)reg;
  (void)name;
  (void)intf;
  (void)v;
}
/* For removing a global variable */
void reg_glob_rem(
    void* data,
    struct wl_registry* reg,
    u32 name
) {
  (void)data;
  (void)reg;
  (void)name;
}
/* For allocating shared memory */
i32 alloc_shm(u64 size) {
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
void frame_new(void* data, struct wl_callback* callback, u32 a) {
  wl_callback_destroy(state.callback);
  state.callback = wl_surface_frame(state.surf);
  wl_callback_add_listener(callback, &state.cb_list, NULL);
  draw();
  (void)data;
  (void)callback;
  (void)a;
}
/* Handle resize */
void resize(void) {
  u32 size = state.width * state.height * 4;
  i32 fd = alloc_shm(size);
  state.pixels =
    mmap(
        0,
        size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );
  struct wl_shm_pool* pool = wl_shm_create_pool(state.shm, fd, size);
  state.buff = wl_shm_pool_create_buffer(
      pool,
      0,
      state.width,
      state.height,
      state.width * 4,
      WL_SHM_FORMAT_ARGB8888
  );
  wl_shm_pool_destroy(pool);
  close(fd);
}
/* XDG surfact config */
void xsrf_conf(void* data, struct xdg_surface* xsrf, u32 ser) {
  xdg_surface_ack_configure(xsrf, ser);
  if (!state.pixels) resize();
  draw();
  (void)data;
}
/* XDG toplevel config */
void top_conf(
    void* data,
    struct xdg_toplevel* top, 
    i32 w, i32 h,
    struct wl_array* states
) {
  if (!w && !h) return;
  if (w != state.width || h != state.height) {
    munmap(state.pixels, state.width * state.height * 4);
    state.width = w;
    state.height = h;
    resize();
  }
  (void)data;
  (void)top;
  (void)w;
  (void)h;
  (void)states;
}
/* XDG toplevel close */
void top_close(void* data, struct xdg_toplevel* top) {
  (void)data;
  (void)top;
}
/* Ping for XDG base */
void sh_ping(void* data, struct xdg_wm_base* sh, u32 ser) {
  xdg_wm_base_pong(sh, ser);
  (void)data;
  (void)sh;
  (void)ser;
}
/* Draw screen */
void draw(void) {
  for (u16 y = 0; y < state.height; y++)
    for (u16 x = 0; x < state.width; x++)
      putpixel(x, y, 0xff000000);
  for (u16 i = 0; i < 100; i++)
    putpixel(i, i, 0xff00ff00);
  wl_surface_attach(state.surf, state.buff, 0, 0);
  wl_surface_damage(state.surf, 0, 0, state.width, state.height);
  wl_surface_commit(state.surf);
}
/* Put one pixel to the screen */
void putpixel(u16 x, u16 y, u32 pixel) {
  ((u32*)state.pixels)[y * state.width + x] = pixel;
}
