// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_SERVER_H
#define NEDM_SERVER_H

#include "config.h"
#include "ipc_server.h"
#include "message.h"
#include "status_bar.h"
#include "wallpaper.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

struct nedm_seat;
struct nedm_output;
struct keybinding_list;
struct wlr_output_layout;
struct wlr_idle_inhibit_manager_v1;
struct nedm_output_config;
struct nedm_input_manager;
struct nedm_layer_shell;

struct nedm_server {
	struct wl_display *wl_display;
	struct wl_event_loop *event_loop;

	struct nedm_seat *seat;
	struct nedm_input_manager *input;
	struct wlr_backend *backend;
	struct wlr_idle_notifier_v1 *idle;
	struct wlr_idle_inhibit_manager_v1 *idle_inhibit_v1;
	struct wl_listener new_idle_inhibitor_v1;
	struct wlr_gamma_control_manager_v1 *gamma_control;
	struct wl_listener gamma_control_set_gamma;
	struct wl_list inhibitors;

	struct wlr_output_layout *output_layout;
	struct wlr_scene_output_layout *scene_output_layout;
	struct wl_list disabled_outputs;
	struct wl_list outputs;
	struct nedm_output *curr_output;
	struct wl_listener new_output;
	struct wl_list output_priorities;
	struct wlr_backend *headless_backend;
	struct wlr_session *session;

	struct wlr_renderer *renderer;
	struct wlr_allocator *allocator;
	struct wlr_scene *scene;

	struct wl_listener xdg_toplevel_decoration;
	struct wl_listener new_xdg_shell_toplevel;
	struct wl_list xdg_decorations;
	
	struct nedm_layer_shell *layer_shell;
#if NEDM_HAS_XWAYLAND
	struct wl_listener new_xwayland_surface;
	struct wlr_xwayland *xwayland;
#endif

	struct keybinding_list *keybindings;
	struct wl_list output_config;
	struct wl_list input_config;
	struct nedm_message_config message_config;
	struct nedm_status_bar_config status_bar_config;
	struct nedm_wallpaper_config wallpaper_config;

	struct nedm_ipc_handle ipc;

	bool enable_socket;
	bool bs;
	bool running;
	char **modes;
	char **modecursors;
	uint16_t nws;
	float *bg_color;
	uint32_t views_curr_id;
	uint32_t tiles_curr_id;
	uint32_t xcursor_size;
};

void
display_terminate(struct nedm_server *server);
int
get_mode_index_from_name(char *const *modes, const char *mode_name);
char *
server_show_info(struct nedm_server *server);

#endif
