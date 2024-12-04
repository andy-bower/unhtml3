/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#ifndef _PARSE_GUMBO_H
#define _PARSE_GUMBO_H
#ifndef WITH_GUMBO
#define GUMBO_PARSERS
#else

#include "unhtml.h"

#define GUMBO_PARSERS &parser_tagsoup,

extern int parse_tagsoup(struct mapped_buffer *input);

static const struct parser_defn parser_tagsoup = {
  .name       = "tagsoup",
  .parse_fn   = parse_tagsoup,
  .imatch_pat = "<!DOCTYPE +html( +SYSTEM +\"about:legacy-compat\")? *>",
};

#endif
#endif
