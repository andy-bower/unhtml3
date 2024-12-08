/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#include <fcntl.h>
#include <glob.h>
#include <errno.h>
#include <search.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <uchar.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "unhtml.h"
#include "config.h"

struct config config;

static const char8_t *config_ns = u8"tag:sw.cdefg.uk,2024:unhtml/config";

static int element_compar(const void *a, const void *b) {
  struct render_elem *aa = (struct render_elem *) a;
  struct render_elem *bb = (struct render_elem *) b;
  return xmlStrcmp(aa->tag, bb->tag);
}

static enum op get_op(xmlNode *node) {
  const xmlChar *attr = xmlGetProp(node, u8"op");
  if (!xmlStrcmp(attr, u8"replace"))
    return OP_REPLACE;
  else
    return OP_ADD;
}

static enum spacing get_spacing(xmlNode *node) {
  const xmlChar *attr = xmlGetProp(node, u8"spacing");
  if (!xmlStrcmp(attr, u8"para"))
    return SPACING_PARA;
  else if (!xmlStrcmp(attr, u8"newline"))
    return SPACING_NEWLINE;
  else
    return OP_ADD;
}

static bool get_skip(xmlNode *node) {
  const xmlChar *attr = xmlGetProp(node, u8"skip");
  if (!xmlStrcmp(attr, u8"skip"))
    return true;
  else
    return false;
}

static void print_action(const void *node, VISIT which, int depth) {
  struct render_elem *r;

  switch (which) {
  case postorder:
  case leaf:
    r = *((struct render_elem **) node);
    logvv(" - element: %s\n", r->tag);
  default:
    break;
  }
}

void print_config(void) {
  logvv("loaded config:\n");
  if (opt.verbosity >= 2)
    twalk(config.elements, print_action);
}

int load_config_file(xmlParserCtxtPtr ctx, const char *file) {
  xmlDocPtr doc;
  xmlNode *root;
  int rc = 1;
  int options = 0;

  if ((doc = xmlCtxtReadFile(ctx, file,
                             NULL, options)) == NULL)
    goto fail1;

  if ((root = xmlDocGetRootElement(doc)) == NULL)
    goto fail2;

  if (!xmlStrcmp(config_ns, root->ns->href)) {
    xmlNode *node;
    enum op op;
    logv("reading config file %s\n", file);
    for (node = root->children; node; node = node->next) {
      xmlNode *atom;
      if (node->type == XML_ELEMENT_NODE &&
          !xmlStrcmp(node->name, u8"elements")) {
        op = get_op(node);
        if (op == OP_REPLACE)
          tdestroy(config.elements, free);
        for (atom = node->children; atom; atom = atom->next) {
          if (atom->type == XML_ELEMENT_NODE &&
              !xmlStrcmp(atom->name, u8"element")) {
            xmlChar *attr;
            size_t attr_len;
            attr = xmlGetProp(atom, u8"tag");
            attr_len = xmlStrlen(attr);
            if (attr) {
              struct render_elem *r = malloc(sizeof *r + attr_len + 1);
              void *result;

              r->spacing = get_spacing(atom);
              r->skip = get_skip(atom);
              memcpy(r->tag, attr, attr_len + 1);
              result = tsearch(r, &config.elements, element_compar);
              if (result == NULL)
                logv("error storing element rendering, %s\n", strerror(errno));
              else
                logvv("stored element %s\n", (char *) r->tag);
            } else {
              logv("no tag specified for element in config\n");
            }
          }
        }
      }
    }
    rc = 0;
  } else {
    logv("ignoring config file using namespace %s\n", root->ns->href);
  }

fail2:
  xmlFreeDoc(doc);

fail1:
  if (rc != 0)
    fprintf(stderr, "xml parsing failed\n");

  print_config();

  return rc;
}

int load_config(struct config_dir *dirs) {
  struct config_dir *dir;
  xmlParserCtxtPtr ctx;
  glob_t glob_buf;
  int flags = 0;
  int rc = 1;
  int i;

  for (dir = dirs; dir; dir = dir->next) {
    char *path;

    asprintf(&path, "%s/*.xml", dir->dir);
    logv("looking for config files: %s\n", path);
    rc = glob(path, flags, NULL, &glob_buf);
    if (rc != 0 && rc != GLOB_NOMATCH) {
      perror("glob(): reading configs");
      exit(1);
    }
    flags |= GLOB_APPEND;
  }

  if (glob_buf.gl_pathc == 0 ||
      (ctx = xmlNewParserCtxt()) == NULL)
    goto fail;

  for (i = 0; i < glob_buf.gl_pathc; i++) {
    load_config_file(ctx, glob_buf.gl_pathv[i]);
  }

  xmlFreeParserCtxt(ctx);

fail:
  globfree(&glob_buf);

  return rc;
};

struct render_elem *get_rendering(const char8_t *tag) {
  /* The following object is only safe to use to refer to the key field, 'tag'.
   * any other usage risks reading invalid or unmapped data. */
  struct render_elem *dummy = ((struct render_elem *) (tag - offsetof(struct render_elem, tag)));
  void *node = tfind(dummy, &config.elements, element_compar);
  return node ? *((struct render_elem **) node) : nullptr;
}
