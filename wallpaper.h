// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_WALLPAPER_H
#define NEDM_WALLPAPER_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>
#include <cairo.h>

struct nedm_server;
struct nedm_output;

enum nedm_wallpaper_mode {
	NEDM_WALLPAPER_FILL,
	NEDM_WALLPAPER_FIT,
	NEDM_WALLPAPER_STRETCH,
	NEDM_WALLPAPER_CENTER,
	NEDM_WALLPAPER_TILE,
};

struct nedm_wallpaper_config {
	char *image_path;
	enum nedm_wallpaper_mode mode;
	float bg_color[4]; // fallback color if image fails to load
};

struct nedm_wallpaper {
	struct wlr_scene_buffer *scene_buffer;
	struct nedm_output *output;
	
	cairo_surface_t *image_surface;
	cairo_surface_t *render_surface;
	cairo_t *cairo;
	
	char *image_path;
	enum nedm_wallpaper_mode mode;
	
	uint32_t output_width;
	uint32_t output_height;
	uint32_t image_width;
	uint32_t image_height;
	
	struct wl_listener output_destroy;
	
	bool loaded;
};

void nedm_wallpaper_init(struct nedm_server *server);
void nedm_wallpaper_destroy(struct nedm_wallpaper *wallpaper);
void nedm_wallpaper_create_for_output(struct nedm_output *output);
void nedm_wallpaper_render(struct nedm_wallpaper *wallpaper);
bool nedm_wallpaper_load_image(struct nedm_wallpaper *wallpaper, const char *path);

#endif