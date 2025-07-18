// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_LAYER_SHELL_H
#define NEDM_LAYER_SHELL_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_scene.h>

struct nedm_server;
struct nedm_output;

struct nedm_layer_shell {
	struct wlr_layer_shell_v1 *layer_shell;
	struct wl_listener new_surface;
	struct wl_listener destroy;
};

struct nedm_layer_surface {
	struct wlr_layer_surface_v1 *layer_surface;
	struct wlr_scene_layer_surface_v1 *scene_layer_surface;
	struct nedm_output *output;
	
	struct wl_listener destroy;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener commit;
	struct wl_listener new_popup;
};

struct nedm_layer_popup {
	struct wlr_xdg_popup *wlr_popup;
	struct wlr_scene_tree *scene_tree;
	struct nedm_layer_surface *parent;
	
	struct wl_listener commit;
	struct wl_listener destroy;
	struct wl_listener new_popup;
	struct wl_listener reposition;
};

void nedm_layer_shell_init(struct nedm_server *server);
void nedm_layer_shell_destroy(struct nedm_layer_shell *layer_shell);
void nedm_arrange_layers(struct nedm_output *output);

#endif