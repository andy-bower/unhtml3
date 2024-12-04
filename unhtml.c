/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

/* unhtml: remove HTML tags from document, emitting plain text.
 *
 * Version 3.0 onwards of unhtml is a complete rewrite in 2024 of v2.3.9 of
 * unhtml written by Kevin Swan in 1998. No portion of the original software
 * remains but the user interface and behaviour is compatible.
 */

#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "unhtml.h"
#include "load.h"
#include "parse-gumbo.h"
#include "parse-libxml2.h"

#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

enum opt:int {
  OPT_VERSION = 0x1000,
  OPT_HELP,
  OPT_COMMENT,
  OPT_CDATA,
  OPT_PARSER,
};

struct options opt;

struct parser parsers[] = {
LIBXML2_PARSERS
GUMBO_PARSERS
};

static constexpr size_t num_parsers = sizeof parsers/sizeof *parsers;

static const char *version_str = STRINGIFY(UNHTML_VERSION);

static void usage(FILE *out) {
  fprintf(out,
          "usage: %s -version              show version information\n"
          "       %s -help                 show help\n"
          "       %s [OPTIONS] [FILENAME]  process FILENAME or stdin\n\n"
          "OPTIONS\n"
          "  -comment        include comments\n"
          "  -cdata=comment  treat CDATA sections as comment\n"
          "  -cdata=text     treat CDATA sections as text (default)\n"
          "  -parser=PARSER  use PARSER parser\n",
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

void list_parsers(FILE *stream) {
  fprintf(stream, "Parsers:\n");
  for (int i = 0; i < num_parsers; i++)
    fprintf(stream, "  %s\n", parsers[i].name);
}

struct parser *find_parser(const char *name) {
  int i;
  for (i = 0; i < num_parsers && strcmp(parsers[i].name, name); i++);
  return i == num_parsers ? nullptr : parsers + i;
}

static void parse_options(int argc, char *argv[]) {
  const struct option options[] = {
    { "version", no_argument,       0, OPT_VERSION },
    { "help",    no_argument,       0, OPT_HELP },
    { "comment", no_argument,       0, OPT_COMMENT },
    { "cdata",   required_argument, 0, OPT_CDATA },
    { "parser",  required_argument, 0, OPT_PARSER },
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
    case OPT_PARSER:
      if (!(opt.parser = find_parser(optarg))) {
        fprintf(stderr, "no such parser: %s\n", optarg);
        list_parsers(stderr);
        opt.error = true;
      }
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

int main(int argc, char *argv[]) {
  struct mapped_buffer input;
  size_t max_buf;
  int rc;

  max_buf = max_input_buffer();
  parse_options(argc, argv);

  /* Choose default parser */
  if (opt.parser == nullptr)
    opt.parser = parsers + 0;

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
    list_parsers(stdout);
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

  rc = opt.parser->parse_fn(&input);

  free_map(&input);

  return EXIT_SUCCESS;
}
