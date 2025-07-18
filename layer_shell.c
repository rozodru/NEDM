// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#include "layer_shell.h"
#include "output.h"
#include "server.h"
#include "util.h"

#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/util/region.h>

static void layer_popup_handle_commit(struct wl_listener *listener, void *data) {
	(void)data;
	struct nedm_layer_popup *popup = wl_container_of(listener, popup, commit);
	if (popup->wlr_popup->base->initial_commit) {
		wlr_xdg_popup_unconstrain_from_box(popup->wlr_popup, NULL);
	}
}

static void layer_popup_handle_destroy(struct wl_listener *listener, void *data) {
	(void)data;
	struct nedm_layer_popup *popup = wl_container_of(listener, popup, destroy);
	wl_list_remove(&popup->commit.link);
	wl_list_remove(&popup->destroy.link);
	wl_list_remove(&popup->new_popup.link);
	wl_list_remove(&popup->reposition.link);
	free(popup);
}

static void layer_popup_handle_reposition(struct wl_listener *listener, void *data) {
	(void)data;
	struct nedm_layer_popup *popup = wl_container_of(listener, popup, reposition);
	wlr_scene_node_set_position(&popup->scene_tree->node, 
		popup->wlr_popup->current.geometry.x,
		popup->wlr_popup->current.geometry.y);
}

static void layer_popup_handle_new_popup(struct wl_listener *listener, void *data) {
	struct nedm_layer_popup *popup = wl_container_of(listener, popup, new_popup);
	struct wlr_xdg_popup *wlr_popup = data;
	
	struct nedm_layer_popup *new_popup = calloc(1, sizeof(struct nedm_layer_popup));
	if (!new_popup) {
		wlr_log(WLR_ERROR, "Failed to allocate layer popup");
		return;
	}
	
	new_popup->wlr_popup = wlr_popup;
	new_popup->parent = popup->parent;
	new_popup->scene_tree = wlr_scene_xdg_surface_create(
		popup->scene_tree, wlr_popup->base);
	
	new_popup->commit.notify = layer_popup_handle_commit;
	wl_signal_add(&wlr_popup->base->surface->events.commit, &new_popup->commit);
	
	new_popup->destroy.notify = layer_popup_handle_destroy;
	wl_signal_add(&wlr_popup->events.destroy, &new_popup->destroy);
	
	new_popup->new_popup.notify = layer_popup_handle_new_popup;
	wl_signal_add(&wlr_popup->base->events.new_popup, &new_popup->new_popup);
	
	new_popup->reposition.notify = layer_popup_handle_reposition;
	wl_signal_add(&wlr_popup->events.reposition, &new_popup->reposition);
}

static void layer_surface_handle_commit(struct wl_listener *listener, void *data) {
	(void)data;
	struct nedm_layer_surface *surface = wl_container_of(listener, surface, commit);
	
	if (surface->layer_surface->initial_commit) {
		wlr_layer_surface_v1_configure(surface->layer_surface,
			surface->layer_surface->current.desired_width,
			surface->layer_surface->current.desired_height);
	}
}

static void layer_surface_handle_destroy(struct wl_listener *listener, void *data) {
	(void)data;
	struct nedm_layer_surface *surface = wl_container_of(listener, surface, destroy);
	
	wl_list_remove(&surface->destroy.link);
	wl_list_remove(&surface->map.link);
	wl_list_remove(&surface->unmap.link);
	wl_list_remove(&surface->commit.link);
	wl_list_remove(&surface->new_popup.link);
	
	if (surface->output) {
		nedm_arrange_layers(surface->output);
	}
	
	free(surface);
}

static void layer_surface_handle_map(struct wl_listener *listener, void *data) {
	(void)data;
	struct nedm_layer_surface *surface = wl_container_of(listener, surface, map);
	if (surface->output) {
		nedm_arrange_layers(surface->output);
	}
}

static void layer_surface_handle_unmap(struct wl_listener *listener, void *data) {
	(void)data;
	struct nedm_layer_surface *surface = wl_container_of(listener, surface, unmap);
	if (surface->output) {
		nedm_arrange_layers(surface->output);
	}
}

static void layer_surface_handle_new_popup(struct wl_listener *listener, void *data) {
	struct nedm_layer_surface *surface = wl_container_of(listener, surface, new_popup);
	struct wlr_xdg_popup *wlr_popup = data;
	
	struct nedm_layer_popup *popup = calloc(1, sizeof(struct nedm_layer_popup));
	if (!popup) {
		wlr_log(WLR_ERROR, "Failed to allocate layer popup");
		return;
	}
	
	popup->wlr_popup = wlr_popup;
	popup->parent = surface;
	popup->scene_tree = wlr_scene_xdg_surface_create(
		surface->scene_layer_surface->tree, wlr_popup->base);
	
	popup->commit.notify = layer_popup_handle_commit;
	wl_signal_add(&wlr_popup->base->surface->events.commit, &popup->commit);
	
	popup->destroy.notify = layer_popup_handle_destroy;
	wl_signal_add(&wlr_popup->events.destroy, &popup->destroy);
	
	popup->new_popup.notify = layer_popup_handle_new_popup;
	wl_signal_add(&wlr_popup->base->events.new_popup, &popup->new_popup);
	
	popup->reposition.notify = layer_popup_handle_reposition;
	wl_signal_add(&wlr_popup->events.reposition, &popup->reposition);
}

static void layer_shell_handle_new_surface(struct wl_listener *listener, void *data) {
	struct nedm_layer_shell *layer_shell = wl_container_of(listener, layer_shell, new_surface);
	struct wlr_layer_surface_v1 *layer_surface = data;
	
	wlr_log(WLR_DEBUG, "New layer surface: namespace %s layer %d anchor %d "
		"size %dx%d margin %d,%d,%d,%d",
		layer_surface->namespace, layer_surface->pending.layer, 
		layer_surface->pending.anchor,
		layer_surface->pending.desired_width, layer_surface->pending.desired_height,
		layer_surface->pending.margin.top, layer_surface->pending.margin.right,
		layer_surface->pending.margin.bottom, layer_surface->pending.margin.left);
	
	struct nedm_layer_surface *surface = calloc(1, sizeof(struct nedm_layer_surface));
	if (!surface) {
		wlr_log(WLR_ERROR, "Failed to allocate layer surface");
		return;
	}
	
	surface->layer_surface = layer_surface;
	layer_surface->data = surface;
	
	// Find output for this layer surface
	struct nedm_server *server = layer_shell->layer_shell->data;
	struct nedm_output *output = NULL;
	
	if (layer_surface->output) {
		struct wlr_output *wlr_output = layer_surface->output;
		output = wlr_output->data;
	} else {
		// If no output specified, use the first available output
		if (!wl_list_empty(&server->outputs)) {
			output = wl_container_of(server->outputs.next, output, link);
			layer_surface->output = output->wlr_output;
		}
	}
	
	if (!output) {
		wlr_log(WLR_ERROR, "No output available for layer surface");
		free(surface);
		return;
	}
	
	surface->output = output;
	
	// Create scene layer surface
	surface->scene_layer_surface = wlr_scene_layer_surface_v1_create(
		output->layers[layer_surface->pending.layer], layer_surface);
	
	surface->destroy.notify = layer_surface_handle_destroy;
	wl_signal_add(&layer_surface->events.destroy, &surface->destroy);
	
	surface->map.notify = layer_surface_handle_map;
	wl_signal_add(&layer_surface->surface->events.map, &surface->map);
	
	surface->unmap.notify = layer_surface_handle_unmap;
	wl_signal_add(&layer_surface->surface->events.unmap, &surface->unmap);
	
	surface->commit.notify = layer_surface_handle_commit;
	wl_signal_add(&layer_surface->surface->events.commit, &surface->commit);
	
	surface->new_popup.notify = layer_surface_handle_new_popup;
	wl_signal_add(&layer_surface->events.new_popup, &surface->new_popup);
	
	// Initial configuration
	wlr_layer_surface_v1_configure(layer_surface, 0, 0);
}

static void layer_shell_handle_destroy(struct wl_listener *listener, void *data) {
	(void)data;
	struct nedm_layer_shell *layer_shell = wl_container_of(listener, layer_shell, destroy);
	wl_list_remove(&layer_shell->new_surface.link);
	wl_list_remove(&layer_shell->destroy.link);
	free(layer_shell);
}

void nedm_layer_shell_init(struct nedm_server *server) {
	struct nedm_layer_shell *layer_shell = calloc(1, sizeof(struct nedm_layer_shell));
	if (!layer_shell) {
		wlr_log(WLR_ERROR, "Failed to allocate layer shell");
		return;
	}
	
	layer_shell->layer_shell = wlr_layer_shell_v1_create(server->wl_display, 4);
	layer_shell->layer_shell->data = server;
	
	layer_shell->new_surface.notify = layer_shell_handle_new_surface;
	wl_signal_add(&layer_shell->layer_shell->events.new_surface, &layer_shell->new_surface);
	
	layer_shell->destroy.notify = layer_shell_handle_destroy;
	wl_signal_add(&layer_shell->layer_shell->events.destroy, &layer_shell->destroy);
	
	server->layer_shell = layer_shell;
}

void nedm_layer_shell_destroy(struct nedm_layer_shell *layer_shell) {
	if (!layer_shell) {
		return;
	}
	
	wl_list_remove(&layer_shell->new_surface.link);
	wl_list_remove(&layer_shell->destroy.link);
	free(layer_shell);
}

void nedm_arrange_layers(struct nedm_output *output) {
	if (!output) {
		return;
	}
	
	struct wlr_box full_area = {0};
	full_area.width = output->wlr_output->width;
	full_area.height = output->wlr_output->height;
	
	struct wlr_box usable_area = full_area;
	
	// Arrange layers from background to overlay
	// Each layer tree's layer surfaces will be positioned according to their configuration
	for (int i = 0; i < 4; i++) {
		if (!output->layers[i]) {
			continue;
		}
		
		// Iterate through layer surfaces in this layer
		struct wlr_scene_node *node;
		wl_list_for_each(node, &output->layers[i]->children, link) {
			if (node->type == WLR_SCENE_NODE_TREE) {
				struct wlr_scene_tree *tree = wlr_scene_tree_from_node(node);
				if (tree->node.data) {
					struct wlr_scene_layer_surface_v1 *scene_layer_surface = tree->node.data;
					wlr_scene_layer_surface_v1_configure(scene_layer_surface, &full_area, &usable_area);
				}
			}
		}
	}
}