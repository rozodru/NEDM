// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_MESSAGE_H

#define NEDM_MESSAGE_H

#include <wayland-server-core.h>

struct nedm_output;
struct wlr_box;
struct wlr_buffer;

enum nedm_message_anchor {
	NEDM_MESSAGE_TOP_LEFT,
	NEDM_MESSAGE_TOP_CENTER,
	NEDM_MESSAGE_TOP_RIGHT,
	NEDM_MESSAGE_BOTTOM_LEFT,
	NEDM_MESSAGE_BOTTOM_CENTER,
	NEDM_MESSAGE_BOTTOM_RIGHT,
	NEDM_MESSAGE_CENTER,
	NEDM_MESSAGE_NOPT
};

struct nedm_message_config {
	char *font;
	int display_time;
	float bg_color[4];
	float fg_color[4];
	int enabled;
	enum nedm_message_anchor anchor;
};

struct nedm_message {
	struct wlr_box *position;
	struct wlr_scene_buffer *message;
	struct wl_surface *surface;
	struct msg_buffer *buf;
	struct wl_list link;
};

void
message_printf(struct nedm_output *output, const char *fmt, ...);
void
message_printf_pos(struct nedm_output *output, struct wlr_box *position,
                   enum nedm_message_anchor, const char *fmt, ...);
void
message_clear(struct nedm_output *output);

#endif /* end of include guard NEDM_MESSAGE_H */
