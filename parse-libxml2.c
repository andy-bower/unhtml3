/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

/* parse HTML
 *
 * Uses libxml to parse the HTML, walks the resultant tree and renders
 * only text content. The output is in UTF-8.
 */

#include <uchar.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>

#include "config.h"
#include "render.h"
#include "parse-libxml2.h"

static void walk_tree(xmlNode *node) {
  /* By default, neither render content nor descend tree further */
  bool follow = false;
  bool content = false;
  const struct render_elem *rendering = nullptr;

  switch (node->type) {
  case XML_CDATA_SECTION_NODE:
    if (!opt.cdata_is_comment || opt.comment)
      content = true;
    break;
  case XML_COMMENT_NODE:
    if (opt.comment)
      content = true;
    break;
  case XML_TEXT_NODE:
    content = true;
    break;
  case XML_DOCUMENT_NODE:
    follow = true;
    break;
  case XML_ELEMENT_NODE:
    rendering = get_rendering(node->name);
    follow = true;
    break;
  default:
    /* Do nothing */
  }

  if (content)
    render_text(node->content);

  if (follow) {
    render_element(node->name, false, rendering);
    if (!rendering || !rendering->skip)
      for (xmlNode *child = node->children; child; child = child->next)
        walk_tree(child);
    render_element(node->name, true, rendering);
  }
}

int parse_html(struct mapped_buffer *input) {
  htmlParserCtxtPtr ctx;
  htmlDocPtr doc;
  xmlNode *root;
  int rc = 1;
  int options =
    HTML_PARSE_NOERROR |
    HTML_PARSE_NOWARNING;

  if ((ctx = htmlNewParserCtxt()) == NULL)
    goto fail1;

  if ((doc = htmlCtxtReadMemory(ctx,
                                input->data,
                                input->length - 1,
                                input->uri,
                                NULL, options)) == NULL)
    goto fail2;

  if ((root = xmlDocGetRootElement(doc)) == NULL)
    goto fail3;

  walk_tree(root);
  rc = 0;

fail3:
  xmlFreeDoc(doc);

fail2:
  htmlFreeParserCtxt(ctx);

fail1:
  if (rc != 0)
    fprintf(stderr, "html parsing failed\n");

  return rc;
}

int parse_xml(struct mapped_buffer *input) {
  xmlParserCtxtPtr ctx;
  xmlDocPtr doc;
  xmlNode *root;
  int rc = 1;
  int options = XML_PARSE_DTDLOAD;

  if ((ctx = xmlNewParserCtxt()) == NULL)
    goto fail1;

  if ((doc = xmlCtxtReadMemory(ctx,
                               input->data,
                               input->length - 1,
                               input->uri,
                               NULL, options)) == NULL)
    goto fail2;

  if ((root = xmlDocGetRootElement(doc)) == NULL)
    goto fail3;

  walk_tree(root);
  rc = 0;

fail3:
  xmlFreeDoc(doc);

fail2:
  xmlFreeParserCtxt(ctx);

fail1:
  if (rc != 0)
    fprintf(stderr, "xml parsing failed\n");

  return rc;
}
