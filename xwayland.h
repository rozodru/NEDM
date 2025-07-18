// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_XWAYLAND_H
#define NEDM_XWAYLAND_H

#include "view.h"

struct nedm_xwayland_view {
	struct nedm_view view;
	struct wlr_xwayland_surface *xwayland_surface;
	struct wlr_scene_tree *scene_tree;

	struct wl_listener destroy;
	struct wl_listener unmap;
	struct wl_listener map;
	struct wl_listener associate;
	struct wl_listener dissociate;
	struct wl_listener request_fullscreen;
};

struct nedm_xwayland_view *
xwayland_view_from_view(struct nedm_view *view);
bool
xwayland_view_should_manage(const struct nedm_view *view);
void
handle_xwayland_surface_new(struct wl_listener *listener, void *data);

#endif
