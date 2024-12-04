/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#ifndef _LOAD_H
#define _LOAD_H

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

extern int map_file(struct mapped_buffer *map_ret, size_t max, const char *file);
extern int map_stream(struct mapped_buffer *map_ret, size_t max, FILE *stream);
extern void free_map(struct mapped_buffer *map);

#endif
