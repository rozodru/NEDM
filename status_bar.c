// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#include "status_bar.h"
#include "output.h"
#include "server.h"
#include "util.h"
#include "workspace.h"

#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/interfaces/wlr_buffer.h>
#include <wlr/types/wlr_shm.h>
#include <drm_fourcc.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>

#define STATUS_BAR_MARGIN 8
#define DEFAULT_HEIGHT 24
#define DEFAULT_FONT "monospace 10"
#define DEFAULT_UPDATE_INTERVAL 1000 // 1 second

struct status_bar_buffer {
	struct wlr_buffer base;
	void *data;
	uint32_t format;
	size_t stride;
};

static void
status_bar_buffer_destroy(struct wlr_buffer *wlr_buffer) {
	struct status_bar_buffer *buffer = wl_container_of(wlr_buffer, buffer, base);
	free(buffer->data);
	free(buffer);
}

static bool
status_bar_buffer_begin_data_ptr_access(struct wlr_buffer *wlr_buffer,
                                 __attribute__((unused)) uint32_t flags,
                                 void **data, uint32_t *format,
                                 size_t *stride) {
	struct status_bar_buffer *buffer = wl_container_of(wlr_buffer, buffer, base);
	if(data != NULL) {
		*data = (void *)buffer->data;
	}
	if(format != NULL) {
		*format = buffer->format;
	}
	if(stride != NULL) {
		*stride = buffer->stride;
	}
	return true;
}

static void
status_bar_buffer_end_data_ptr_access(
    __attribute__((unused)) struct wlr_buffer *wlr_buffer) {
	// This space is intentionally left blank
}

static const struct wlr_buffer_impl status_bar_buffer_impl = {
    .destroy = status_bar_buffer_destroy,
    .begin_data_ptr_access = status_bar_buffer_begin_data_ptr_access,
    .end_data_ptr_access = status_bar_buffer_end_data_ptr_access,
};

static struct status_bar_buffer *
status_bar_buffer_create(uint32_t width, uint32_t height, uint32_t stride) {
	struct status_bar_buffer *buffer = calloc(1, sizeof(*buffer));
	if(buffer == NULL) {
		return NULL;
	}
	size_t size = stride * height;
	buffer->data = malloc(size);
	if(buffer->data == NULL) {
		free(buffer);
		return NULL;
	}
	buffer->format = DRM_FORMAT_ARGB8888;
	buffer->stride = stride;
	wlr_buffer_init(&buffer->base, &status_bar_buffer_impl, width, height);
	return buffer;
}

static struct wlr_scene_buffer *create_status_bar_buffer(struct nedm_status_bar *status_bar) {
	struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_create(
		status_bar->output->layers[2], NULL);
	
	if (!scene_buffer) {
		wlr_log(WLR_ERROR, "Failed to create scene buffer for status bar");
		return NULL;
	}
	
	return scene_buffer;
}

static void status_bar_gather_system_info(struct nedm_status_bar *status_bar, struct nedm_status_info *info) {
	time_t now;
	struct tm *tm_info;
	char time_buffer[32];
	char date_buffer[32];
	
	// Get current time
	time(&now);
	tm_info = localtime(&now);
	strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", tm_info);
	strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", tm_info);
	
	free(info->time_str);
	free(info->date_str);
	info->time_str = strdup(time_buffer);
	info->date_str = strdup(date_buffer);
	
	// Battery info
	FILE *battery_file = fopen("/sys/class/power_supply/BAT0/capacity", "r");
	if (battery_file) {
		fscanf(battery_file, "%d", &info->battery_percent);
		fclose(battery_file);
		
		free(info->battery_str);
		info->battery_str = malloc(16);
		snprintf(info->battery_str, 16, "BAT: %d%%", info->battery_percent);
	} else {
		free(info->battery_str);
		info->battery_str = strdup("BAT: N/A");
		info->battery_percent = -1;
	}
	
	// Charging status
	FILE *charging_file = fopen("/sys/class/power_supply/BAT0/status", "r");
	if (charging_file) {
		char status[16];
		fgets(status, sizeof(status), charging_file);
		info->charging = (strncmp(status, "Charging", 8) == 0);
		fclose(charging_file);
	} else {
		info->charging = false;
	}
	
	// Volume info - get actual volume percentage
	free(info->volume_str);
	info->volume_str = malloc(32);
	
	FILE *vol_pipe = popen("amixer get Master | grep -o '[0-9]*%' | head -1", "r");
	if (vol_pipe) {
		char vol_buffer[16];
		if (fgets(vol_buffer, sizeof(vol_buffer), vol_pipe)) {
			vol_buffer[strcspn(vol_buffer, "\n")] = 0;
			snprintf(info->volume_str, 32, "VOL: %s", vol_buffer);
		} else {
			snprintf(info->volume_str, 32, "VOL: ??");
		}
		pclose(vol_pipe);
	} else {
		snprintf(info->volume_str, 32, "VOL: N/A");
	}
	
	// WiFi info (simplified)
	FILE *wifi_file = fopen("/proc/net/wireless", "r");
	if (wifi_file) {
		char line[256];
		info->wifi_connected = false;
		while (fgets(line, sizeof(line), wifi_file)) {
			if (strstr(line, "wlan") || strstr(line, "wlp")) {
				info->wifi_connected = true;
				break;
			}
		}
		fclose(wifi_file);
		
		free(info->wifi_str);
		info->wifi_str = strdup(info->wifi_connected ? "WIFI: ON" : "WIFI: OFF");
	} else {
		free(info->wifi_str);
		info->wifi_str = strdup("WIFI: N/A");
		info->wifi_connected = false;
	}
	
	// Workspace info - get actual current workspace from output
	free(info->workspace_str);
	if (status_bar && status_bar->output) {
		info->workspace_str = malloc(16);
		snprintf(info->workspace_str, 16, "WS: %d", status_bar->output->curr_workspace + 1);
	} else {
		info->workspace_str = strdup("WS: ?");
	}
}

static void status_bar_free_info(struct nedm_status_info *info) {
	free(info->time_str);
	free(info->date_str);
	free(info->battery_str);
	free(info->volume_str);
	free(info->wifi_str);
	free(info->workspace_str);
	memset(info, 0, sizeof(*info));
}

static void status_bar_render_text(struct nedm_status_bar *status_bar, const char *text, int x, int y) {
	if (!text || !status_bar->cairo || !status_bar->pango_layout) {
		return;
	}
	
	pango_layout_set_text(status_bar->pango_layout, text, -1);
	cairo_move_to(status_bar->cairo, x, y);
	pango_cairo_show_layout(status_bar->cairo, status_bar->pango_layout);
}

void nedm_status_bar_render(struct nedm_status_bar *status_bar) {
	if (!status_bar->cairo_surface || !status_bar->cairo) {
		return;
	}
	
	struct nedm_status_bar_config *config = &status_bar->output->server->status_bar_config;
	
	// Clear background
	cairo_set_source_rgba(status_bar->cairo, 
		config->bg_color[0], config->bg_color[1], config->bg_color[2], config->bg_color[3]);
	cairo_paint(status_bar->cairo);
	
	// Set text color
	cairo_set_source_rgba(status_bar->cairo, 
		config->text_color[0], config->text_color[1], config->text_color[2], config->text_color[3]);
	
	// Gather system information
	struct nedm_status_info info = {0};
	status_bar_gather_system_info(status_bar, &info);
	
	// Calculate positions for right-aligned text
	int current_x = status_bar->width - STATUS_BAR_MARGIN;
	int y = (status_bar->height - 12) / 2; // Center vertically
	
	// Render components from right to left
	if (info.time_str && config->show_time) {
		PangoRectangle text_rect;
		pango_layout_set_text(status_bar->pango_layout, info.time_str, -1);
		pango_layout_get_pixel_extents(status_bar->pango_layout, NULL, &text_rect);
		current_x -= text_rect.width;
		status_bar_render_text(status_bar, info.time_str, current_x, y);
		current_x -= STATUS_BAR_MARGIN;
	}
	
	if (info.date_str && config->show_date) {
		PangoRectangle text_rect;
		pango_layout_set_text(status_bar->pango_layout, info.date_str, -1);
		pango_layout_get_pixel_extents(status_bar->pango_layout, NULL, &text_rect);
		current_x -= text_rect.width;
		status_bar_render_text(status_bar, info.date_str, current_x, y);
		current_x -= STATUS_BAR_MARGIN;
	}
	
	if (info.battery_str && config->show_battery) {
		PangoRectangle text_rect;
		pango_layout_set_text(status_bar->pango_layout, info.battery_str, -1);
		pango_layout_get_pixel_extents(status_bar->pango_layout, NULL, &text_rect);
		current_x -= text_rect.width;
		
		// Color code battery
		if (info.battery_percent >= 0) {
			if (info.charging) {
				cairo_set_source_rgba(status_bar->cairo, 0.0, 1.0, 0.0, 1.0); // Green when charging
			} else if (info.battery_percent < 20) {
				cairo_set_source_rgba(status_bar->cairo, 1.0, 0.0, 0.0, 1.0); // Red when low
			} else if (info.battery_percent < 50) {
				cairo_set_source_rgba(status_bar->cairo, 1.0, 1.0, 0.0, 1.0); // Yellow when medium
			} else {
				cairo_set_source_rgba(status_bar->cairo, 1.0, 1.0, 1.0, 1.0); // White when good
			}
		}
		
		status_bar_render_text(status_bar, info.battery_str, current_x, y);
		cairo_set_source_rgba(status_bar->cairo, 
			config->text_color[0], config->text_color[1], config->text_color[2], config->text_color[3]);
		current_x -= STATUS_BAR_MARGIN;
	}
	
	if (info.volume_str && config->show_volume) {
		PangoRectangle text_rect;
		pango_layout_set_text(status_bar->pango_layout, info.volume_str, -1);
		pango_layout_get_pixel_extents(status_bar->pango_layout, NULL, &text_rect);
		current_x -= text_rect.width;
		status_bar_render_text(status_bar, info.volume_str, current_x, y);
		current_x -= STATUS_BAR_MARGIN;
	}
	
	if (info.wifi_str && config->show_wifi) {
		PangoRectangle text_rect;
		pango_layout_set_text(status_bar->pango_layout, info.wifi_str, -1);
		pango_layout_get_pixel_extents(status_bar->pango_layout, NULL, &text_rect);
		current_x -= text_rect.width;
		
		// Color code WiFi
		if (info.wifi_connected) {
			cairo_set_source_rgba(status_bar->cairo, 0.0, 1.0, 0.0, 1.0); // Green when connected
		} else {
			cairo_set_source_rgba(status_bar->cairo, 1.0, 0.0, 0.0, 1.0); // Red when disconnected
		}
		
		status_bar_render_text(status_bar, info.wifi_str, current_x, y);
		cairo_set_source_rgba(status_bar->cairo, 
			config->text_color[0], config->text_color[1], config->text_color[2], config->text_color[3]);
		current_x -= STATUS_BAR_MARGIN;
	}
	
	if (info.workspace_str && config->show_workspace) {
		PangoRectangle text_rect;
		pango_layout_set_text(status_bar->pango_layout, info.workspace_str, -1);
		pango_layout_get_pixel_extents(status_bar->pango_layout, NULL, &text_rect);
		current_x -= text_rect.width;
		status_bar_render_text(status_bar, info.workspace_str, current_x, y);
	}
	
	// Clean up
	status_bar_free_info(&info);
	
	// Update the scene buffer with the new Cairo surface content
	if (status_bar->scene_buffer) {
		cairo_surface_flush(status_bar->cairo_surface);
		unsigned char *data = cairo_image_surface_get_data(status_bar->cairo_surface);
		int width = cairo_image_surface_get_width(status_bar->cairo_surface);
		int height = cairo_image_surface_get_height(status_bar->cairo_surface);
		int stride = cairo_image_surface_get_stride(status_bar->cairo_surface);
		
		struct status_bar_buffer *buf = status_bar_buffer_create(width, height, stride);
		if (!buf) return;
		
		void *data_ptr;
		if(!wlr_buffer_begin_data_ptr_access(&buf->base,
		                                     WLR_BUFFER_DATA_PTR_ACCESS_WRITE,
		                                     &data_ptr, NULL, NULL)) {
			wlr_log(WLR_ERROR, "Failed to get pointer access to status bar buffer");
			return;
		}
		memcpy(data_ptr, data, stride * height);
		wlr_buffer_end_data_ptr_access(&buf->base);
		
		wlr_scene_buffer_set_buffer(status_bar->scene_buffer, &buf->base);
		wlr_buffer_drop(&buf->base);
	}
	
}

static int status_bar_timer_callback(void *data) {
	struct nedm_status_bar *status_bar = data;
	
	// Validate status bar structure before rendering
	if (!status_bar || !status_bar->output || !status_bar->output->server) {
		return 0; // Stop timer
	}
	
	if (!status_bar->mapped) {
		return 0; // Stop timer
	}
	
	nedm_status_bar_render(status_bar);
	// Manually reschedule the timer instead of returning interval
	wl_event_source_timer_update(status_bar->timer, status_bar->output->server->status_bar_config.update_interval);
	
	return 0; // Always return 0, we handle rescheduling manually
}

static void status_bar_handle_output_destroy(struct wl_listener *listener, void *data) {
	(void)data;
	struct nedm_status_bar *status_bar = wl_container_of(listener, status_bar, output_destroy);
	nedm_status_bar_destroy(status_bar);
}

void nedm_status_bar_create_for_output(struct nedm_output *output) {
	if (!output || !output->server) {
		wlr_log(WLR_ERROR, "Invalid output or server for status bar creation");
		return;
	}
	
	struct nedm_status_bar *status_bar = calloc(1, sizeof(struct nedm_status_bar));
	if (!status_bar) {
		wlr_log(WLR_ERROR, "Failed to allocate status bar");
		return;
	}
	
	status_bar->output = output;
	output->status_bar = status_bar;
	
	struct nedm_status_bar_config *config = &output->server->status_bar_config;
	
	// Calculate dimensions using configuration
	int output_width = output->wlr_output->width;
	int output_height = output->wlr_output->height;
	status_bar->width = (output_width * config->width_percent) / 100;
	status_bar->height = config->height;
	
	// Create Cairo surface
	status_bar->cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 
		status_bar->width, status_bar->height);
	status_bar->cairo = cairo_create(status_bar->cairo_surface);
	
	// Create Pango layout
	status_bar->pango_layout = pango_cairo_create_layout(status_bar->cairo);
	const char *font_str = config->font ? config->font : DEFAULT_FONT;
	status_bar->font_desc = pango_font_description_from_string(font_str);
	pango_layout_set_font_description(status_bar->pango_layout, status_bar->font_desc);
	
	// Render the status bar text first
	nedm_status_bar_render(status_bar);
	
	// Create the status bar buffer from Cairo surface
	status_bar->scene_buffer = create_status_bar_buffer(status_bar);
	if (!status_bar->scene_buffer) {
		wlr_log(WLR_ERROR, "Failed to create scene buffer for status bar");
		nedm_status_bar_destroy(status_bar);
		return;
	}
	
	// Position the status bar based on configuration
	int x, y;
	switch (config->position) {
	case NEDM_STATUS_BAR_TOP_LEFT:
		x = 0;
		y = 0;
		break;
	case NEDM_STATUS_BAR_TOP_RIGHT:
		x = output_width - status_bar->width;
		y = output_height - status_bar->height; // TEMPORARILY MOVED TO BOTTOM-RIGHT TO TEST NOTIFICATION CONFLICT
		break;
	case NEDM_STATUS_BAR_BOTTOM_LEFT:
		x = 0;
		y = output_height - status_bar->height;
		break;
	case NEDM_STATUS_BAR_BOTTOM_RIGHT:
		x = output_width - status_bar->width;
		y = output_height - status_bar->height;
		break;
	default:
		x = output_width - status_bar->width;
		y = output_height - status_bar->height; // MOVED TO BOTTOM-RIGHT TO TEST NOTIFICATION CONFLICT
		break;
	}
	wlr_scene_node_set_position(&status_bar->scene_buffer->node, x, y);
	
	// Set up event listeners
	status_bar->output_destroy.notify = status_bar_handle_output_destroy;
	wl_signal_add(&output->events.destroy, &status_bar->output_destroy);
	
	// Set up timer for updates
	status_bar->timer = wl_event_loop_add_timer(
		output->server->event_loop, status_bar_timer_callback, status_bar);
	wl_event_source_timer_update(status_bar->timer, config->update_interval);
	
	// Initial render
	nedm_status_bar_render(status_bar);
	
	status_bar->mapped = true;
	
	wlr_log(WLR_INFO, "Created status bar for output %s", output->wlr_output->name);
}

void nedm_status_bar_destroy(struct nedm_status_bar *status_bar) {
	if (!status_bar) {
		return;
	}
	
	if (status_bar->timer) {
		wl_event_source_remove(status_bar->timer);
	}
	
	if (status_bar->scene_buffer) {
		wlr_scene_node_destroy(&status_bar->scene_buffer->node);
	}
	
	if (status_bar->font_desc) {
		pango_font_description_free(status_bar->font_desc);
	}
	
	if (status_bar->pango_layout) {
		g_object_unref(status_bar->pango_layout);
	}
	
	if (status_bar->cairo) {
		cairo_destroy(status_bar->cairo);
	}
	
	if (status_bar->cairo_surface) {
		cairo_surface_destroy(status_bar->cairo_surface);
	}
	
	wl_list_remove(&status_bar->output_destroy.link);
	
	if (status_bar->output) {
		status_bar->output->status_bar = NULL;
	}
	
	free(status_bar);
}

void nedm_status_bar_init(struct nedm_server *server) {
	(void)server;
	// Status bars are created per-output, so nothing to initialize globally
	wlr_log(WLR_INFO, "Status bar subsystem initialized");
}