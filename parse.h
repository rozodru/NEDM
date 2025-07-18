// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_PARSE_H

#define NEDM_PARSE_H

#include <stdio.h>

struct nedm_server;

int
parse_rc_line(struct nedm_server *server, char *line, char **errstr);
char *
parse_malloc_vsprintf(const char *fmt, ...);
char *
parse_malloc_vsprintf_va_list(const char *fmt, va_list list);

#endif /* end of include guard NEDM_PARSE_H */
