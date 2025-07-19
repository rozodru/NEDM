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
	
	if (surface->layer_surface->initial_commit && surface->layer_surface->initialized) {
		// Applications expect a configure response
		uint32_t width = surface->layer_surface->current.desired_width;
		uint32_t height = surface->layer_surface->current.desired_height;
		
		if (width == 0) width = 100;  // Default width if not specified
		if (height == 0) height = 100; // Default height if not specified
		
		wlr_layer_surface_v1_configure(surface->layer_surface, width, height);
	}
	
	if (surface->output) {
		nedm_arrange_layers(surface->output);
	}
}

static void layer_surface_handle_destroy(struct wl_listener *listener, void *data) {
	(void)data;
	struct nedm_layer_surface *surface = wl_container_of(listener, surface, destroy);
	
	// Remove from server's layer surfaces list
	wl_list_remove(&surface->link);
	
	wl_list_remove(&surface->destroy.link);
	
	// Only remove surface event listeners if they were added
	if (surface->layer_surface && surface->layer_surface->surface) {
		if (!wl_list_empty(&surface->map.link)) {
			wl_list_remove(&surface->map.link);
		}
		if (!wl_list_empty(&surface->unmap.link)) {
			wl_list_remove(&surface->unmap.link);
		}
		if (!wl_list_empty(&surface->commit.link)) {
			wl_list_remove(&surface->commit.link);
		}
	}
	
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
	
	wlr_log(WLR_ERROR, "=== LAYER SHELL: New surface created ===");
	wlr_log(WLR_ERROR, "namespace: %s", layer_surface->namespace);
	wlr_log(WLR_ERROR, "client: %p", layer_surface->resource ? wl_resource_get_client(layer_surface->resource) : NULL);
	
	struct nedm_layer_surface *surface = calloc(1, sizeof(struct nedm_layer_surface));
	if (!surface) {
		wlr_log(WLR_ERROR, "Failed to allocate layer surface");
		return;
	}
	
	surface->layer_surface = layer_surface;
	layer_surface->data = surface;
	
	// Initialize listener links
	wl_list_init(&surface->map.link);
	wl_list_init(&surface->unmap.link);
	wl_list_init(&surface->commit.link);
	
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
	
	// Add to server's layer surfaces list
	wl_list_insert(&server->layer_surfaces, &surface->link);
	
	// Create scene layer surface
	wlr_log(WLR_ERROR, "Creating scene layer surface for layer %d", layer_surface->pending.layer);
	surface->scene_layer_surface = wlr_scene_layer_surface_v1_create(
		output->layers[layer_surface->pending.layer], layer_surface);
	if (!surface->scene_layer_surface) {
		wlr_log(WLR_ERROR, "FAILED to create scene layer surface");
		free(surface);
		return;
	}
	wlr_log(WLR_ERROR, "Successfully created scene layer surface %p", surface->scene_layer_surface);
	
	surface->destroy.notify = layer_surface_handle_destroy;
	wl_signal_add(&layer_surface->events.destroy, &surface->destroy);
	
	surface->map.notify = layer_surface_handle_map;
	if (layer_surface->surface) {
		wl_signal_add(&layer_surface->surface->events.map, &surface->map);
	}
	
	surface->unmap.notify = layer_surface_handle_unmap;
	if (layer_surface->surface) {
		wl_signal_add(&layer_surface->surface->events.unmap, &surface->unmap);
	}
	
	surface->commit.notify = layer_surface_handle_commit;
	if (layer_surface->surface) {
		wl_signal_add(&layer_surface->surface->events.commit, &surface->commit);
	}
	
	surface->new_popup.notify = layer_surface_handle_new_popup;
	wl_signal_add(&layer_surface->events.new_popup, &surface->new_popup);
	
	// Arrange layers to ensure proper positioning
	nedm_arrange_layers(output);
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
	if (!layer_shell->layer_shell) {
		wlr_log(WLR_ERROR, "Failed to create layer shell v1");
		free(layer_shell);
		return;
	}
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

static void arrange_layer(struct nedm_output *output, int layer_index, 
                         struct wlr_box *full_area, struct wlr_box *usable_area, bool exclusive) {
	if (!output->layers[layer_index]) {
		return;
	}
	
	// Instead of searching the scene tree, iterate through our stored layer surfaces
	// Find all nedm_layer_surface instances for this output and layer
	struct nedm_server *server = output->server;
	struct nedm_layer_surface *surface;
	
	// We need to iterate through all layer surfaces and find ones that match this output and layer
	// This is a temporary solution - ideally we'd store them in a list per output/layer
	wl_list_for_each(surface, &server->layer_surfaces, link) {
		if (surface->output != output) {
			continue;
		}
		
		if (!surface->scene_layer_surface || !surface->layer_surface) {
			continue;
		}
		
		if (surface->layer_surface->current.layer != layer_index) {
			continue;
		}
		
		struct wlr_scene_layer_surface_v1 *scene_layer_surface = surface->scene_layer_surface;
		
		struct wlr_layer_surface_v1 *layer_surface = scene_layer_surface->layer_surface;
		
		wlr_log(WLR_ERROR, "Found layer surface: namespace='%s' exclusive_zone=%d anchor=%d size=%dx%d", 
			layer_surface->namespace, layer_surface->current.exclusive_zone,
			layer_surface->current.anchor, layer_surface->current.actual_width, layer_surface->current.actual_height);
		
		// Only arrange surfaces with exclusive zones in the exclusive pass
		// and non-exclusive surfaces in the non-exclusive pass
		if ((layer_surface->current.exclusive_zone > 0) != exclusive) {
			wlr_log(WLR_ERROR, "Skipping surface (exclusive_zone=%d, exclusive_pass=%s)", 
				layer_surface->current.exclusive_zone, exclusive ? "true" : "false");
			continue;
		}
		
		if (!scene_layer_surface->layer_surface->initialized) {
			wlr_log(WLR_ERROR, "Skipping uninitialized layer surface");
			continue;
		}
		
		wlr_log(WLR_ERROR, "CONFIGURING layer surface: full_area=%d,%d %dx%d usable_area=%d,%d %dx%d",
			full_area->x, full_area->y, full_area->width, full_area->height,
			usable_area->x, usable_area->y, usable_area->width, usable_area->height);
		wlr_scene_layer_surface_v1_configure(scene_layer_surface, full_area, usable_area);
	}
}

void nedm_arrange_layers(struct nedm_output *output) {
	if (!output || !output->wlr_output) {
		return;
	}
	
	wlr_log(WLR_ERROR, "NEDM ARRANGE LAYERS: Called for output %s (%dx%d)",
		output->wlr_output->name, output->wlr_output->width, output->wlr_output->height);
	
	struct wlr_box full_area = {
		.x = 0,
		.y = 0,
		.width = output->wlr_output->width,
		.height = output->wlr_output->height
	};
	
	struct wlr_box usable_area = full_area;
	
	// Arrange layers in Sway's order: overlay, top, bottom, background
	// First pass: arrange surfaces with exclusive zones
	arrange_layer(output, 3, &full_area, &usable_area, true); // OVERLAY
	arrange_layer(output, 2, &full_area, &usable_area, true); // TOP
	arrange_layer(output, 1, &full_area, &usable_area, true); // BOTTOM
	arrange_layer(output, 0, &full_area, &usable_area, true); // BACKGROUND
	
	// Second pass: arrange surfaces without exclusive zones
	arrange_layer(output, 3, &full_area, &usable_area, false); // OVERLAY
	arrange_layer(output, 2, &full_area, &usable_area, false); // TOP
	arrange_layer(output, 1, &full_area, &usable_area, false); // BOTTOM
	arrange_layer(output, 0, &full_area, &usable_area, false); // BACKGROUND
}