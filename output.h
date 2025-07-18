// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_OUTPUT_H
#define NEDM_OUTPUT_H

#include <wayland-server-core.h>
#include <wlr/util/box.h>

struct nedm_server;
struct nedm_view;
struct wlr_output;
struct wlr_surface;
struct nedm_status_bar;
struct nedm_wallpaper;

enum output_role {
	OUTPUT_ROLE_PERIPHERAL,
	OUTPUT_ROLE_PERMANENT,
	OUTPUT_ROLE_DEFAULT
};

struct nedm_output {
	struct nedm_server *server;
	struct wlr_output *wlr_output;
	struct wlr_scene_rect *bg;
	struct wlr_scene_output *scene_output;

	struct wl_listener commit;
	struct wl_listener destroy;
	struct wl_listener frame;
	struct nedm_workspace **workspaces;
	struct wl_list messages;
	struct wlr_box layout_box;
	int curr_workspace;
	int priority;
	enum output_role role;
	bool destroyed;
	char *name;
	
	struct wlr_scene_tree *layers[4]; // ZWLR_LAYER_SHELL_V1_LAYER_*
	struct nedm_status_bar *status_bar;
	struct nedm_wallpaper *wallpaper;
	struct {
		struct wl_signal destroy;
	} events;

	struct wl_list link; // nedm_server::outputs
};

struct nedm_output_priorities {
	char *ident;
	int priority;
	struct wl_list link;
};

enum output_status { OUTPUT_ENABLE, OUTPUT_DISABLE, OUTPUT_DEFAULT };

struct nedm_output_config {
	enum output_status status;
	enum output_role role;
	struct wlr_box pos;
	char *output_name;
	float refresh_rate;
	float scale;
	int priority;
	int angle;           // enum wl_output_transform, -1 signifies "unspecified"
	struct wl_list link; // nedm_server::output_config
};

typedef void (*nedm_surface_iterator_func_t)(struct nedm_output *output,
                                           struct wlr_surface *surface,
                                           struct wlr_box *box,
                                           void *user_data);
struct wlr_box
output_get_layout_box(struct nedm_output *output);
void
handle_new_output(struct wl_listener *listener, void *data);
void
output_configure(struct nedm_server *server, struct nedm_output *output);
void
output_set_window_title(struct nedm_output *output, const char *title);
void
output_make_workspace_fullscreen(struct nedm_output *output, uint32_t ws);
int
output_get_num(const struct nedm_output *output);
void
handle_output_gamma_control_set_gamma(struct wl_listener *listener, void *data);
void
output_insert(struct nedm_server *server, struct nedm_output *output);
#endif
