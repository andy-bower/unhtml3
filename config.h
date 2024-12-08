/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#ifndef _CONFIG_H
#define _CONFIG_H

#include "unhtml.h"
#include <uchar.h>
#include <libxml/xmlstring.h>

enum spacing {
  SPACING_NONE,
  SPACING_PARA,
  SPACING_NEWLINE,
  SPACING_SPACE,
};

enum op {
  OP_ADD,
  OP_REPLACE,
};

struct render_elem {
  enum spacing spacing;
  bool skip;
  xmlChar tag[];
};

struct config {
  void *elements;
};

struct config_dir {
  char *dir;
  struct config_dir *next;
  bool name_needs_free:1;
  bool node_needs_free:1;
};

static struct config_dir defconf_package = { PREFIX "/share/" UNHTML, nullptr };
static struct config_dir defconf_system  = { "/etc/" UNHTML, &defconf_package };
static struct config_dir defconf_user    = { NULL, &defconf_system };

static inline struct config_dir *get_defconf(void) {
  const char *dir = getenv("XDG_CONFIG_HOME");
  if (dir) {
    char *path = NULL;

    asprintf(&path, "%s/" UNHTML, dir);
    defconf_user.dir = strdup(path);
    defconf_user.name_needs_free = true;
    return &defconf_user;
  } else {
    return &defconf_system;
  }
}

extern struct config config;

extern int load_config(struct config_dir *dirs);

extern struct render_elem *get_rendering(const char8_t *tag);

#endif
