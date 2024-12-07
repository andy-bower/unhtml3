/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#ifndef _UNHTML_H
#define _UNHTML_H

#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

#ifdef INST_PREFIX
#define PREFIX STRINGIFY(INST_PREFIX)
#else
#define PREFIX "/usr/local"
#endif
#define UNHTML "unhtml"

#include <regex.h>
#include <stdarg.h>

#include "load.h"

enum render_mode {
  RENDER_MODE_LITERAL = 0,
  RENDER_MODE_SMART,
  RENDER_MODE_MAX,
};

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
  int verbosity;
  const char *file;
  int parser;
  struct config_dir *confdirs;
  enum render_mode render_mode;
};

extern struct options opt;

static inline void logv(const char *fmt, ...) {
  va_list args;

  if (opt.verbosity >= 1) {
    va_start(args);
    vfprintf(stderr, fmt, args);
    va_end(args);
  }
}

static inline void logvv(const char *fmt, ...) {
  va_list args;

  if (opt.verbosity >= 2) {
    va_start(args);
    vfprintf(stderr, fmt, args);
    va_end(args);
  }
}

#endif
