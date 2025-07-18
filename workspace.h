// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_WORKSPACE_H
#define NEDM_WORKSPACE_H

#include <wlr/util/box.h>

struct nedm_output;
struct nedm_server;

struct nedm_tile {
	struct nedm_workspace *workspace;
	struct wlr_box tile;
	struct nedm_view *view;
	struct nedm_tile *next;
	struct nedm_tile *prev;
	uint32_t id;
};

struct nedm_workspace {
	struct nedm_server *server;
	struct wl_list views;
	struct wl_list unmanaged_views;
	struct nedm_output *output;
	struct wlr_scene_tree *scene;

	struct nedm_tile *focused_tile;
	uint32_t num;
};

struct nedm_workspace *
full_screen_workspace(struct nedm_output *output);
int
full_screen_workspace_tiles(struct nedm_workspace *workspace,
                            uint32_t *tiles_curr_id);
void
workspace_free_tiles(struct nedm_workspace *workspace);
void
workspace_free(struct nedm_workspace *workspace);
void
workspace_focus_tile(struct nedm_workspace *ws, struct nedm_tile *tile);
void
workspace_focus(struct nedm_output *outp, int ws);
void
workspace_tile_update_view(struct nedm_tile *tile, struct nedm_view *view);

#endif
