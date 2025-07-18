// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include "message.h"
#include "output.h"
#include "seat.h"
#include "server.h"
#include "view.h"
#include "workspace.h"

void
workspace_tile_update_view(struct nedm_tile *tile, struct nedm_view *view) {
	if(tile->view != NULL) {
		wlr_scene_node_set_enabled(&tile->view->scene_tree->node, false);
		tile->view->tile = NULL;
	}
	tile->view = view;
	if(view != NULL) {
		view_maximize(view, tile);
		wlr_scene_node_set_enabled(&view->scene_tree->node, true);
	}
}

int
full_screen_workspace_tiles(struct nedm_workspace *workspace,
                            uint32_t *tiles_curr_id) {
	workspace->focused_tile = calloc(1, sizeof(struct nedm_tile));
	if(!workspace->focused_tile) {
		return -1;
	}
	workspace->focused_tile->workspace = workspace;
	workspace->focused_tile->next = workspace->focused_tile;
	workspace->focused_tile->prev = workspace->focused_tile;
	workspace->focused_tile->tile.x = 0;
	workspace->focused_tile->tile.y = 0;
	workspace->focused_tile->tile.width =
	    output_get_layout_box(workspace->output).width;
	workspace->focused_tile->tile.height =
	    output_get_layout_box(workspace->output).height;
	workspace_tile_update_view(workspace->focused_tile, NULL);
	workspace->focused_tile->id = *tiles_curr_id;
	++(*tiles_curr_id);
	return 0;
}

struct nedm_workspace *
full_screen_workspace(struct nedm_output *output) {
	struct nedm_workspace *workspace = calloc(1, sizeof(struct nedm_workspace));
	if(!workspace) {
		return NULL;
	}
	struct wlr_scene_output *scene_output =
	    wlr_scene_get_scene_output(output->server->scene, output->wlr_output);
	if(scene_output == NULL) {
		free(workspace);
		return NULL;
	}
	workspace->output = output;
	workspace->server = output->server;
	workspace->num = -1;
	workspace->scene = wlr_scene_tree_create(&scene_output->scene->tree);
	if(full_screen_workspace_tiles(workspace, &output->server->tiles_curr_id) !=
	   0) {
		free(workspace);
		return NULL;
	}
	return workspace;
}

void
workspace_focus_tile(struct nedm_workspace *ws, struct nedm_tile *tile) {
	ws->focused_tile = tile;
	struct wlr_box *box = malloc(sizeof(struct wlr_box));
	if(!box) {
		wlr_log(WLR_ERROR, "Failed to allocate box required to focus tile");
		return;
	}
	box->x = tile->tile.x + tile->tile.width / 2;
	box->y = tile->tile.y + tile->tile.height / 2;
	message_printf_pos(ws->output, box, NEDM_MESSAGE_CENTER, "Current tile");
}

void
workspace_free_tiles(struct nedm_workspace *workspace) {
	workspace->focused_tile->prev->next = NULL;
	while(workspace->focused_tile != NULL) {
		if(workspace->server->seat != NULL &&
		   (!workspace->output->server->running ||
		    workspace->focused_tile == workspace->server->seat->cursor_tile)) {
			workspace->server->seat->cursor_tile = NULL;
		}
		struct nedm_tile *next = workspace->focused_tile->next;
		free(workspace->focused_tile);
		workspace->focused_tile = next;
	}
}

void
workspace_free(struct nedm_workspace *workspace) {
	wlr_scene_node_destroy(&workspace->scene->node);
	workspace_free_tiles(workspace);
	free(workspace);
}

void
workspace_focus(struct nedm_output *outp, int ws) {
	if(ws >= outp->server->nws) {
		wlr_log(WLR_ERROR,
		        "Attempt to focus workspace %d, but only %d workspaces are "
		        "available.",
		        ws, outp->server->nws);
		return;
	}
	wlr_scene_node_place_above(
	    &outp->bg->node, &outp->workspaces[outp->curr_workspace]->scene->node);
	wlr_scene_node_place_above(&outp->workspaces[ws]->scene->node,
	                           &outp->bg->node);
	
	// Ensure proper layer ordering: background(0) -> bottom(1) -> workspace -> top(2) -> overlay(3)
	// Background layer should be below workspace content, but visible
	if (outp->layers[0]) {
		wlr_scene_node_place_below(&outp->layers[0]->node, &outp->workspaces[ws]->scene->node);
	}
	if (outp->layers[1]) {
		wlr_scene_node_place_below(&outp->layers[1]->node, &outp->workspaces[ws]->scene->node);
	}
	// Top and overlay layers should be above workspace content
	if (outp->layers[2]) {
		wlr_scene_node_place_above(&outp->layers[2]->node, &outp->workspaces[ws]->scene->node);
	}
	if (outp->layers[3]) {
		wlr_scene_node_place_above(&outp->layers[3]->node, 
		                           outp->layers[2] ? &outp->layers[2]->node : &outp->workspaces[ws]->scene->node);
	}
	
	outp->curr_workspace = ws;
}
