// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_XDG_SHELL_H
#define NEDM_XDG_SHELL_H

#include "view.h"

struct nedm_xdg_shell_view {
	struct nedm_view view;
	struct wlr_xdg_toplevel *toplevel;

	struct wl_listener destroy;
	struct wl_listener commit;
	struct wl_listener unmap;
	struct wl_listener map;
	struct wl_listener new_popup;
	struct wl_listener request_fullscreen;
};

struct nedm_xdg_shell_popup {
	struct nedm_view *view;
	struct wlr_xdg_popup *wlr_popup;
	struct wlr_scene_tree *scene_tree;
	struct wlr_scene_tree *xdg_surface_tree;

	struct wl_listener commit;
	struct wl_listener new_popup;
	struct wl_listener reposition;
	struct wl_listener destroy;
};

struct nedm_xdg_decoration {
	struct wlr_xdg_toplevel_decoration_v1 *wlr_decoration;
	struct nedm_server *server;
	struct wl_listener destroy;
	struct wl_listener request_mode;
	struct wl_list link; // server::xdg_decorations
};

void
handle_xdg_shell_toplevel_new(struct wl_listener *listener, void *data);

void
handle_xdg_shell_popup_new(struct wl_listener *listener, void *data);

void
handle_xdg_toplevel_decoration(struct wl_listener *listener, void *data);

#endif
