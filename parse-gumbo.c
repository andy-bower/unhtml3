/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

/* parse tag soup AKA HTML5
 *
 * Uses libgumbo to parse the HTML, walks the resultant tree and renders
 * only text content. The output is in UTF-8.
 */

#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <gumbo.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include "parse-gumbo.h"

static void walk_tree(GumboNode *node) {
  /* By default, neither render content nor descend tree further */
  GumboVector *children = nullptr;
  GumboText *text = nullptr;

  switch (node->type) {
  case GUMBO_NODE_CDATA:
    if (!opt.cdata_is_comment || opt.comment)
      text = &node->v.text;
    break;
  case GUMBO_NODE_COMMENT:
    if (opt.comment)
      text = &node->v.text;
    break;
  case GUMBO_NODE_TEXT:
  case GUMBO_NODE_WHITESPACE:
    text = &node->v.text;
    break;
  case GUMBO_NODE_DOCUMENT:
    children = &node->v.document.children;
    break;
  case GUMBO_NODE_ELEMENT:
    switch (node->v.element.tag) {
    case GUMBO_TAG_SCRIPT:
    case GUMBO_TAG_STYLE:
      /* Do not render content of these tags */
      break;
    default:
      children = &node->v.element.children;
    }
    break;
  default:
    /* Do nothing */
  }

  if (text)
      fputs(text->text, stdout);

  if (children)
    for (int child = 0; child < children->length; child++)
      walk_tree((GumboNode *) children->data[child]);
}

int parse_tagsoup(struct mapped_buffer *input) {
  GumboOutput *doc;

  doc = gumbo_parse(input->data);
  if (doc) {
    walk_tree(doc->root);
    gumbo_destroy_output(&kGumboDefaultOptions, doc);
  } else {
    fprintf(stderr, "html parsing failed\n");
  }

  return 0;
}
