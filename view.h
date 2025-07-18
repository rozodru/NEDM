// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_VIEW_H
#define NEDM_VIEW_H

#include "config.h"

#include <stdbool.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_compositor.h>

struct nedm_server;
struct wlr_box;

enum nedm_view_type {
	NEDM_XDG_SHELL_VIEW,
#if NEDM_HAS_XWAYLAND
	NEDM_XWAYLAND_VIEW,
#endif
};

struct nedm_view {
	struct nedm_workspace *workspace;
	struct nedm_server *server;
	struct wl_list link; // server::views
	struct wlr_surface *wlr_surface;
	struct nedm_tile *tile;
	struct wlr_scene_tree *scene_tree;

	struct wl_listener destroy;
	struct wl_listener unmap;
	struct wl_listener map;

	/* The view has a position in output coordinates. */
	int ox, oy;

	enum nedm_view_type type;
	const struct nedm_view_impl *impl;

	uint32_t id;
};

struct nedm_view_impl {
	pid_t (*get_pid)(const struct nedm_view *view);
	char *(*get_title)(const struct nedm_view *view);
	bool (*is_primary)(const struct nedm_view *view);
	void (*activate)(struct nedm_view *view, bool activate);
	void (*close)(struct nedm_view *view);
	void (*maximize)(struct nedm_view *view, int width, int height);
	void (*destroy)(struct nedm_view *view);
};

struct nedm_tile *
view_get_tile(const struct nedm_view *view);
bool
view_is_primary(const struct nedm_view *view);
bool
view_is_visible(const struct nedm_view *view);
void
view_activate(struct nedm_view *view, bool activate);
void
view_unmap(struct nedm_view *view);
void
view_maximize(struct nedm_view *view, struct nedm_tile *tile);
void
view_map(struct nedm_view *view, struct wlr_surface *surface,
         struct nedm_workspace *ws);
void
view_destroy(struct nedm_view *view);
void
view_init(struct nedm_view *view, enum nedm_view_type type,
          const struct nedm_view_impl *impl, struct nedm_server *server);
struct nedm_view *
view_get_prev_view(struct nedm_view *view);

#endif
