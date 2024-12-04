/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include "unhtml.h"
#include "load.h"

static void write_resource_uri(struct mapped_buffer *map, const char *fmt, ...) {
  va_list args;
  char *result;

  va_start(args);
  vasprintf(&result, fmt, args);
  va_end(args);
  map->uri = strdup(result);
}

int map_file(struct mapped_buffer *map_ret, size_t max, const char *file) {
  struct mapped_buffer map = { };
  struct stat statbuf;
  int rc;

  if ((map.fd = open(file, O_RDONLY)) == -1) {
    fprintf(stderr, "could not open %s, %s\n", file, strerror(errno));
    goto finish;
  }
  rc = stat(file, &statbuf);
  if (rc == -1) {
    fprintf(stderr, "could not stat opened file %s, %s\n",
            file, strerror(errno));
    goto fail;
  }

  /* Zero-terminate the input */
  map.length = statbuf.st_size + 1;

  if (map.length > max) {
    fprintf(stderr, "file too big (%zd)\n", map.length);
    goto fail;
  }
  map.data = mmap(nullptr, map.mapped = map.length,
                  PROT_READ, MAP_PRIVATE, map.fd, 0);
  if (map.data == NULL) {
    fprintf(stderr, "failed to mmap file %s, %s\n", file, strerror(errno));
    goto fail;
  }

  write_resource_uri(&map, "file:///%s", file);

  *map_ret = map;
  return 0;

fail:
  close(map.fd);
finish:
  return 1;
}

int map_stream(struct mapped_buffer *map_ret, size_t max, FILE *stream) {
  struct mapped_buffer map = { .fd = -1 };

  map.data = mmap(nullptr, map.mapped = max,
                  PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (map.data == MAP_FAILED) {
    fprintf(stderr, "failed to map buffer to stash input, %s\n",
            strerror(errno));
    return 1;
  }

  map.length = fread(map.data, 1, max, stdin);
  if (ferror(stdin)) {
    fprintf(stderr, "error reading input, %s\n", strerror(errno));
    return 1;
  }

  /* Zero-terminate the input */
  map.data[map.length++] = '\0';

  write_resource_uri(&map, "file:///%s", "/dev/stdin");

  *map_ret = map;
  return 0;
}

void free_map(struct mapped_buffer *map) {
  munmap(map->data, map->mapped);
  if (map->fd != -1)
    close(map->fd);

  if (map->uri != nullptr)
    free(map->uri);
}
