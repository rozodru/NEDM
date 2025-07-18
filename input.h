// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef _NEDM_INPUT_H
#define _NEDM_INPUT_H

#include <stdbool.h>

struct nedm_input_device;
struct nedm_input_config;
struct nedm_server;

void
nedm_input_configure_libinput_device(struct nedm_input_device *device);

void
nedm_input_apply_config(struct nedm_input_config *config, struct nedm_server *server);

bool
nedm_libinput_device_is_builtin(struct nedm_input_device *device);

#endif
