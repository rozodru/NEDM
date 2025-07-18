// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#include "wallpaper.h"
#include "output.h"
#include "server.h"
#include "util.h"

#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>

#include <cairo.h>
#include <cairo/cairo.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


static void wallpaper_calculate_scaling(struct nedm_wallpaper *wallpaper, 
		double *scale_x, double *scale_y, double *offset_x, double *offset_y) {
	
	double img_w = wallpaper->image_width;
	double img_h = wallpaper->image_height;
	double out_w = wallpaper->output_width;
	double out_h = wallpaper->output_height;
	
	*scale_x = 1.0;
	*scale_y = 1.0;
	*offset_x = 0.0;
	*offset_y = 0.0;
	
	switch (wallpaper->mode) {
		case NEDM_WALLPAPER_FILL: {
			// Scale to fill the entire output, cropping if necessary
			double scale = fmax(out_w / img_w, out_h / img_h);
			*scale_x = scale;
			*scale_y = scale;
			*offset_x = (out_w - img_w * scale) / 2.0;
			*offset_y = (out_h - img_h * scale) / 2.0;
			break;
		}
		case NEDM_WALLPAPER_FIT: {
			// Scale to fit entirely within the output, maintaining aspect ratio
			double scale = fmin(out_w / img_w, out_h / img_h);
			*scale_x = scale;
			*scale_y = scale;
			*offset_x = (out_w - img_w * scale) / 2.0;
			*offset_y = (out_h - img_h * scale) / 2.0;
			break;
		}
		case NEDM_WALLPAPER_STRETCH: {
			// Stretch to fill the entire output, ignoring aspect ratio
			*scale_x = out_w / img_w;
			*scale_y = out_h / img_h;
			*offset_x = 0.0;
			*offset_y = 0.0;
			break;
		}
		case NEDM_WALLPAPER_CENTER: {
			// Center the image without scaling
			*scale_x = 1.0;
			*scale_y = 1.0;
			*offset_x = (out_w - img_w) / 2.0;
			*offset_y = (out_h - img_h) / 2.0;
			break;
		}
		case NEDM_WALLPAPER_TILE: {
			// Tile the image (no scaling, repeat pattern)
			*scale_x = 1.0;
			*scale_y = 1.0;
			*offset_x = 0.0;
			*offset_y = 0.0;
			break;
		}
	}
}

bool nedm_wallpaper_load_image(struct nedm_wallpaper *wallpaper, const char *path) {
	if (!path) {
		wlr_log(WLR_ERROR, "No wallpaper path provided");
		return false;
	}
	
	// Load the image using Cairo
	wallpaper->image_surface = cairo_image_surface_create_from_png(path);
	if (cairo_surface_status(wallpaper->image_surface) != CAIRO_STATUS_SUCCESS) {
		wlr_log(WLR_ERROR, "Failed to load wallpaper image: %s", path);
		if (wallpaper->image_surface) {
			cairo_surface_destroy(wallpaper->image_surface);
			wallpaper->image_surface = NULL;
		}
		return false;
	}
	
	// Get image dimensions
	wallpaper->image_width = cairo_image_surface_get_width(wallpaper->image_surface);
	wallpaper->image_height = cairo_image_surface_get_height(wallpaper->image_surface);
	
	wlr_log(WLR_INFO, "Loaded wallpaper: %s (%dx%d)", path, 
		wallpaper->image_width, wallpaper->image_height);
	
	wallpaper->loaded = true;
	return true;
}

void nedm_wallpaper_render(struct nedm_wallpaper *wallpaper) {
	if (!wallpaper->loaded || !wallpaper->image_surface || !wallpaper->render_surface) {
		return;
	}
	
	struct nedm_wallpaper_config *config = &wallpaper->output->server->wallpaper_config;
	
	// Clear the render surface with configured background color
	cairo_set_source_rgba(wallpaper->cairo, 
		config->bg_color[0], config->bg_color[1], config->bg_color[2], config->bg_color[3]);
	cairo_paint(wallpaper->cairo);
	
	// Calculate scaling and positioning
	double scale_x, scale_y, offset_x, offset_y;
	wallpaper_calculate_scaling(wallpaper, &scale_x, &scale_y, &offset_x, &offset_y);
	
	if (wallpaper->mode == NEDM_WALLPAPER_TILE) {
		// Special handling for tile mode
		cairo_pattern_t *pattern = cairo_pattern_create_for_surface(wallpaper->image_surface);
		cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
		cairo_set_source(wallpaper->cairo, pattern);
		cairo_paint(wallpaper->cairo);
		cairo_pattern_destroy(pattern);
	} else {
		// Apply transformation
		cairo_save(wallpaper->cairo);
		cairo_translate(wallpaper->cairo, offset_x, offset_y);
		cairo_scale(wallpaper->cairo, scale_x, scale_y);
		
		// Draw the image
		cairo_set_source_surface(wallpaper->cairo, wallpaper->image_surface, 0, 0);
		cairo_paint(wallpaper->cairo);
		
		cairo_restore(wallpaper->cairo);
	}
	
	// Flush the surface
	cairo_surface_flush(wallpaper->render_surface);
}

static void wallpaper_handle_output_destroy(struct wl_listener *listener, void *data) {
	(void)data;
	struct nedm_wallpaper *wallpaper = wl_container_of(listener, wallpaper, output_destroy);
	nedm_wallpaper_destroy(wallpaper);
}

void nedm_wallpaper_create_for_output(struct nedm_output *output) {
	if (!output || !output->server) {
		wlr_log(WLR_ERROR, "Invalid output or server for wallpaper creation");
		return;
	}
	
	struct nedm_wallpaper *wallpaper = calloc(1, sizeof(struct nedm_wallpaper));
	if (!wallpaper) {
		wlr_log(WLR_ERROR, "Failed to allocate wallpaper");
		return;
	}
	
	wallpaper->output = output;
	output->wallpaper = wallpaper;
	
	struct nedm_wallpaper_config *config = &output->server->wallpaper_config;
	
	// Set output dimensions
	wallpaper->output_width = output->wlr_output->width;
	wallpaper->output_height = output->wlr_output->height;
	
	// Use configured wallpaper mode
	wallpaper->mode = config->mode;
	
	// Store the image path
	wallpaper->image_path = strdup(config->image_path ? config->image_path : "assets/nedm.png");
	
	// Create render surface
	wallpaper->render_surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 
		wallpaper->output_width, wallpaper->output_height);
	wallpaper->cairo = cairo_create(wallpaper->render_surface);
	
	// Load the wallpaper image
	if (!nedm_wallpaper_load_image(wallpaper, wallpaper->image_path)) {
		wlr_log(WLR_ERROR, "Failed to load wallpaper image");
		nedm_wallpaper_destroy(wallpaper);
		return;
	}
	
	// Render the wallpaper first
	nedm_wallpaper_render(wallpaper);
	
	// Create a scene buffer from the rendered wallpaper
	if (wallpaper->render_surface) {
		// Create a wlr_buffer from the cairo surface
		cairo_surface_flush(wallpaper->render_surface);
		unsigned char *data = cairo_image_surface_get_data(wallpaper->render_surface);
		int stride = cairo_image_surface_get_stride(wallpaper->render_surface);
		
		// For now, create a colored rectangle as fallback
		wallpaper->scene_rect = wlr_scene_rect_create(output->layers[0], // BACKGROUND layer
			wallpaper->output_width, wallpaper->output_height, config->bg_color);
		
		if (!wallpaper->scene_rect) {
			wlr_log(WLR_ERROR, "Failed to create scene rect for wallpaper");
			nedm_wallpaper_destroy(wallpaper);
			return;
		}
		
		// Position the wallpaper at (0, 0) to cover the entire output
		wlr_scene_node_set_position(&wallpaper->scene_rect->node, 0, 0);
	}
	
	// Set up event listeners
	wallpaper->output_destroy.notify = wallpaper_handle_output_destroy;
	wl_signal_add(&output->events.destroy, &wallpaper->output_destroy);
	
	wlr_log(WLR_INFO, "Created wallpaper for output %s (%dx%d) with image %s", 
		output->wlr_output->name, wallpaper->output_width, wallpaper->output_height, 
		wallpaper->image_path);
}

void nedm_wallpaper_destroy(struct nedm_wallpaper *wallpaper) {
	if (!wallpaper) {
		return;
	}
	
	if (wallpaper->scene_rect) {
		wlr_scene_node_destroy(&wallpaper->scene_rect->node);
	}
	
	if (wallpaper->cairo) {
		cairo_destroy(wallpaper->cairo);
	}
	
	if (wallpaper->render_surface) {
		cairo_surface_destroy(wallpaper->render_surface);
	}
	
	if (wallpaper->image_surface) {
		cairo_surface_destroy(wallpaper->image_surface);
	}
	
	if (wallpaper->image_path) {
		free(wallpaper->image_path);
	}
	
	if (wallpaper->output_destroy.notify) {
		wl_list_remove(&wallpaper->output_destroy.link);
	}
	
	if (wallpaper->output) {
		wallpaper->output->wallpaper = NULL;
	}
	
	free(wallpaper);
}

void nedm_wallpaper_init(struct nedm_server *server) {
	(void)server;
	// Wallpapers are created per-output, so nothing to initialize globally
	wlr_log(WLR_INFO, "Wallpaper subsystem initialized");
}