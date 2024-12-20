/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

/* parse tag soup AKA HTML5
 *
 * Uses libgumbo to parse the HTML, walks the resultant tree and renders
 * only text content. The output is in UTF-8.
 */

#include <uchar.h>
#include <gumbo.h>

#include "config.h"
#include "render.h"
#include "parse-gumbo.h"

static void walk_tree(GumboNode *node) {
  /* By default, neither render content nor descend tree further */
  GumboVector *children = nullptr;
  GumboText *text = nullptr;
  const char8_t *tag = nullptr;
  const struct render_elem *rendering = nullptr;

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
    children = &node->v.element.children;
    if (node->v.element.tag < GUMBO_TAG_UNKNOWN) {
      tag = (char8_t *) gumbo_normalized_tagname(node->v.element.tag);
      rendering = get_rendering(tag);
    }
    break;
  default:
    /* Do nothing */
  }

  if (text)
    render_text((char8_t *) text->text);

  if (children) {
    render_element(tag, false, rendering);
    if (!rendering || !rendering->skip)
      for (int child = 0; child < children->length; child++)
        walk_tree((GumboNode *) children->data[child]);
    render_element(tag, true, rendering);
  }
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
