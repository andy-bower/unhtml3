/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

/* unhtml: remove HTML tags from document, emitting plain text.
 *
 * Version 3.0 onwards of unhtml is a complete rewrite in 2024 of v2.3.9 of
 * unhtml written by Kevin Swan in 1998. No portion of the original software
 * remains but the user interface and behaviour is compatible.
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

#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

struct options {
  bool comment;
  bool cdata_is_comment;
  bool error;
  bool version;
  bool help;
  const char *file;
} opt;

enum opt:int {
  OPT_VERSION = 0x1000,
  OPT_HELP,
  OPT_COMMENT,
  OPT_CDATA,
};

static const char *version_str = STRINGIFY(UNHTML_VERSION);

static void usage(FILE *out) {
  fprintf(out,
          "usage: %s -version              show version information\n"
          "       %s -help                 show help\n"
          "       %s [OPTIONS] [FILENAME]  process FILENAME or stdin\n\n"
          "OPTIONS\n"
          "  -comment        include comments\n"
          "  -cdata=comment  treat CDATA sections as comment\n"
          "  -cdata=text     treat CDATA sections as text (default)\n",
          program_invocation_short_name,
          program_invocation_short_name,
          program_invocation_short_name);
}

static void version(FILE *out) {
  fprintf(out,
          "unhtml-%s (c) Copyright 2024, Andrew Bower <andrew@bower.uk>\n"
          "Remove html tags from document and render as UTF-8 plain text.\n"
          "Compatible replacement inspired by unhtml-2.3.9, Kevin Swan 1998.\n",
          version_str);
}

static void parse_options(int argc, char *argv[]) {
  const struct option options[] = {
    { "version", no_argument,       0, OPT_VERSION },
    { "help",    no_argument,       0, OPT_HELP },
    { "comment", no_argument,       0, OPT_COMMENT },
    { "cdata",   required_argument, 0, OPT_CDATA },
    { nullptr }
  };
  int option_index;
  int c;

  memset(&opt, '\0', sizeof opt);
  do {
    c = getopt_long_only(argc, argv, "", options, &option_index);
    switch (c) {
    case OPT_VERSION:
      opt.version = true;
      break;
    case OPT_HELP:
      opt.help = true;
      break;
    case OPT_COMMENT:
      opt.comment = true;
      break;
    case OPT_CDATA:
      if (!strcmp(optarg, "comment"))
        opt.cdata_is_comment = true;
      else if (!strcmp(optarg, "text"))
        opt.cdata_is_comment = false;
      else
        opt.error = true;
      break;
    case -1:
      /* EOF */
      break;
    default:
      opt.error = true;
    }
  } while (c != -1 && c != '?' && c != ':');

  if (c != -1)
    opt.error = true;

  if (optind < argc)
    opt.file = argv[optind++];

  if (optind < argc)
    opt.error = true;
}

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

static size_t max_input_buffer(void) {
  struct rlimit max_mem;
  size_t max_buf;
  int rc;

  rc = getrlimit(RLIMIT_AS, &max_mem);
  if (rc == -1) {
    fprintf(stderr, "could not get limits with getrlimit(), %s\n",
            strerror(errno));
    max_mem.rlim_cur = 0x4000'0000ul;
  }

  /* The html parser will use at least as much memory as the input buffer so
   * limit to a quarter of available virtual memory. */
  max_buf = max_mem.rlim_cur == RLIM_INFINITY ? SIZE_MAX : max_mem.rlim_cur;
  max_buf >>= 2;

  /* According to gumbo.h, the maximum input buffer size is 4GB */
  if (max_buf > 0xFFFF'FFFFul)
    max_buf = 0xFFFF'FFFFul;

  return max_buf;
}

struct mapped_buffer {
  char *data;
  size_t length;
  size_t mapped;
  int fd;
};

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

  *map_ret = map;
  return 0;
}

int main(int argc, char *argv[]) {
  struct mapped_buffer input;
  GumboOutput *doc;
  size_t max_buf;
  int rc;

  max_buf = max_input_buffer();
  parse_options(argc, argv);

  if (opt.error) {
    usage(stderr);
    return EXIT_FAILURE;
  }

  if (opt.help) {
    usage(stdout);
    puts("");
    version(stdout);
    fprintf(stdout,
            "\nMaximum input size: %.1fMB\n",
            max_buf / (1024.0 * 1024.0));
  }

  if (opt.version)
    version(stdout);

  if (opt.help || opt.version)
    return EXIT_SUCCESS;


  if (opt.file) {
    rc = map_file(&input, max_buf, opt.file);
  } else {
    rc = map_stream(&input, max_buf, stdin);
  }

  if (rc != 0)
    return EXIT_FAILURE;

  doc = gumbo_parse(input.data);
  if (doc) {
    walk_tree(doc->root);
    gumbo_destroy_output(&kGumboDefaultOptions, doc);
  } else {
    fprintf(stderr, "html parsing failed\n");
  }

  munmap(input.data, input.mapped);
  if (input.fd != -1)
    close(input.fd);

  return EXIT_SUCCESS;
}
