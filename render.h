/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#ifndef _RENDER_H
#define _RENDER_H

#include "unhtml.h"
#include <uchar.h>
#include <libxml/xmlstring.h>

extern void render_element(const char8_t *tag, bool end, const struct render_elem *rendering);
extern void render_text(const char8_t *text);

#endif
