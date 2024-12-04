/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#ifndef _UNHTML_H
#define _UNHTML_H

#include "load.h"

struct parser {
  char *name;
  int (*parse_fn)(struct mapped_buffer *input);
};

struct options {
  bool comment;
  bool cdata_is_comment;
  bool error;
  bool version;
  bool help;
  const char *file;
  struct parser *parser;
};

extern struct options opt;

#endif
