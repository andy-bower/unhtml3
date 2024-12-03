/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#ifndef _PARSE_UNHTML_H
#define _PARSE_UNHTML_H

#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>

struct mapped_buffer {
  char *data;
  size_t length;
  size_t mapped;
  int fd;
  char *uri;
};

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
