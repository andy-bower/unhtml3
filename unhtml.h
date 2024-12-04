/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#ifndef _UNHTML_H
#define _UNHTML_H

#include <regex.h>

#include "load.h"

struct parser_defn {
  const char *name;
  int (*parse_fn)(struct mapped_buffer *input);
  const char *imatch_pat;
};

struct parser {
  const struct parser_defn *def;
  regex_t match_re;
  bool has_matcher;
};

struct options {
  bool comment;
  bool cdata_is_comment;
  bool error;
  bool version;
  bool help;
  const char *file;
  int parser;
};

extern struct options opt;

#endif
