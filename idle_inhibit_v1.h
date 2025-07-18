// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT
#ifndef NEDM_IDLE_INHIBIT_H
#define NEDM_IDLE_INHIBIT_H

struct wl_listener;

void
handle_idle_inhibitor_v1_new(struct wl_listener *listener, void *data);

#endif
