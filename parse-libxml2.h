/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#ifndef _PARSE_LIBXML2_H
#define _PARSE_LIBXML2_H
#ifndef WITH_LIBXML2
#define LIBXML2_PARSERS
#else

#include "unhtml.h"

#define LIBXML2_PARSERS &parser_html, &parser_xml,

extern int parse_html(struct mapped_buffer *input);
extern int parse_xml(struct mapped_buffer *input);

static const struct parser_defn parser_html = {
  .name       = "html",
  .parse_fn   = parse_html,
  .imatch_pat = "<!DOCTYPE +HTML +PUBLIC +\"-//W3C//DTD +HTML",
};

static const struct parser_defn parser_xml = {
  .name       = "xml",
  .parse_fn   = parse_xml,
  .imatch_pat = "<\\?xml|<!DOCTYPE +html +PUBLIC +\"-//W3C//DTD +XHTML",
};

#endif
#endif
