// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_STATUS_BAR_H
#define NEDM_STATUS_BAR_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_scene.h>
#include <cairo.h>
#include <pango/pangocairo.h>

struct nedm_server;
struct nedm_output;

enum nedm_status_bar_position {
	NEDM_STATUS_BAR_TOP_LEFT,
	NEDM_STATUS_BAR_TOP_RIGHT,
	NEDM_STATUS_BAR_BOTTOM_LEFT,
	NEDM_STATUS_BAR_BOTTOM_RIGHT,
};

struct nedm_status_bar_config {
	enum nedm_status_bar_position position;
	uint32_t height;
	uint32_t width_percent;
	uint32_t update_interval;
	float bg_color[4];
	float text_color[4];
	char *font;
	bool show_time;
	bool show_date;
	bool show_battery;
	bool show_volume;
	bool show_wifi;
	bool show_workspace;
};

struct nedm_status_bar {
	struct wlr_scene_rect *scene_rect;
	struct nedm_output *output;
	
	cairo_surface_t *cairo_surface;
	cairo_t *cairo;
	PangoLayout *pango_layout;
	PangoFontDescription *font_desc;
	
	uint32_t width;
	uint32_t height;
	
	struct wl_listener output_destroy;
	struct wl_event_source *timer;
	
	bool mapped;
};

struct nedm_status_info {
	char *time_str;
	char *date_str;
	char *battery_str;
	char *volume_str;
	char *wifi_str;
	char *workspace_str;
	int battery_percent;
	bool wifi_connected;
	bool charging;
};

void nedm_status_bar_init(struct nedm_server *server);
void nedm_status_bar_destroy(struct nedm_status_bar *status_bar);
void nedm_status_bar_create_for_output(struct nedm_output *output);
void nedm_status_bar_render(struct nedm_status_bar *status_bar);
void nedm_status_bar_update_info(struct nedm_status_bar *status_bar);

#endif