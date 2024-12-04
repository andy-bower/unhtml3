/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#ifndef _PARSE_LIBXML2_H
#define _PARSE_LIBXML2_H
#ifndef WITH_LIBXML2
#define LIBXML2_PARSERS
#else

#include "unhtml.h"

#define LIBXML2_PARSERS \
  { "html", parse_html }, \
  { "xml", parse_xml },

extern int parse_html(struct mapped_buffer *input);
extern int parse_xml(struct mapped_buffer *input);

#endif
#endif
