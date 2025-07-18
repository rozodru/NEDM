// Copyright 2020 - 2025, project-repo and the NEDM contributors
// SPDX-License-Identifier: MIT

#ifndef NEDM_PANGO_H
#define NEDM_PANGO_H
#include <cairo/cairo.h>

void
get_text_size(cairo_t *cairo, const char *font, int *width, int *height,
              int *baseline, double scale, const char *fmt, ...);
void
pango_printf(cairo_t *cairo, const char *font, double scale, const char *fmt,
             ...);

#endif
