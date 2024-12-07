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
#include "config.h"
#include "parse-gumbo.h"
#include "parse-libxml2.h"

enum opt:int {
  OPT_VERSION = 0x1000,
  OPT_HELP,
  OPT_COMMENT,
  OPT_CDATA,
  OPT_PARSER,
  OPT_VERBOSE,
  OPT_CONFDIR,
  OPT_RENDER,
};

struct options opt;

const char *render_mode_names[RENDER_MODE_MAX] = {
  [RENDER_MODE_LITERAL] = "literal",
  [RENDER_MODE_SMART]   = "smart",
};

const struct parser_defn *parser_defs[] = {
LIBXML2_PARSERS
GUMBO_PARSERS
};
static constexpr size_t num_parsers = sizeof parser_defs/sizeof *parser_defs;
static struct parser parsers[num_parsers];

static const char *version_str = STRINGIFY(UNHTML_VERSION);

static void usage(FILE *out) {
  fprintf(out,
          "usage: %s -version              show version information\n"
          "       %s -help                 show help\n"
          "       %s [OPTIONS] [FILENAME]  process FILENAME or stdin\n\n"
          "OPTIONS\n"
          "  -verbose          show verbose output\n"
          "  -comment          include comments\n"
          "  -cdata=comment    treat CDATA sections as comment\n"
          "  -cdata=text       treat CDATA sections as text (default)\n"
          "  -parser=PARSER    use PARSER parser\n"
          "  -confdir=CONFDIR  set configuration search path; subsequently prepend to it\n"
          "  -render=MODE      set rendering mode\n"
          ,
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
    fprintf(stream, "  %s\n", parser_defs[i]->name);
}

void list_render_modes(FILE *stream) {
  fprintf(stream, "Render modes:\n");
  for (int i = 0; i < RENDER_MODE_MAX; i++)
    fprintf(stream, "  %s\n", render_mode_names[i]);
}

int find_parser(const char *name) {
  int i;
  for (i = 0; i < num_parsers && strcmp(parser_defs[i]->name, name); i++);
  return i == num_parsers ? -1 : i;
}

void init_parsers(void) {
  int i;
  memset(&parsers, '\0', sizeof parsers);
  for (i = 0; i < num_parsers; i++) {
    struct parser *p = parsers + i;
    int rc;

    p->def = parser_defs[i];
    if (p->def->imatch_pat) {
      rc = regcomp(&p->match_re, p->def->imatch_pat, REG_EXTENDED | REG_ICASE);
      if (rc != 0)
        logv("error compiling regex for choosing %s parser\n", p->def->name);
      else
        p->has_matcher = true;
    }
  }
}

void parser_match(struct mapped_buffer *input) {
  int i;

  for (i = 0; i < num_parsers; i++) {
    struct parser *p = parsers + i;
    regmatch_t matches[1] = {
      { 0, input->length - 1 }
    };

    /* Require the DOCTYPE or <?xml> prolog to be within the first 1KB of
     * the input. These should occur before any other content anyway but
     * comments can often be found beforehand so let's match liberally. */
    if (matches[0].rm_eo > 1024)
      matches[0].rm_eo = 1024;

    if (p->has_matcher &&
        regexec(&p->match_re,
                input->data,
                0, matches,
                REG_STARTEND) == 0)
      break;
  }

  if (i != num_parsers) {
    opt.parser = i;
    logv("selected '%s' parser based on content\n",
         parsers[i].def->name);
  } else {
    opt.parser = -1;
    logv("no parser matched, using default\n");
  }
}

void free_parsers(void) {
  for (int i = 0; i < num_parsers; i++) {
    if (parsers[i].has_matcher)
      regfree(&parsers[i].match_re);
  }
  memset(parsers, '\0', sizeof parsers);
}

static void parse_options(int argc, char *argv[]) {
  const struct option options[] = {
    { "version", no_argument,       0, OPT_VERSION },
    { "help",    no_argument,       0, OPT_HELP },
    { "comment", no_argument,       0, OPT_COMMENT },
    { "cdata",   required_argument, 0, OPT_CDATA },
    { "parser",  required_argument, 0, OPT_PARSER },
    { "verbose", no_argument,       0, OPT_VERBOSE },
    { "confdir", required_argument, 0, OPT_CONFDIR },
    { "render",  required_argument, 0, OPT_RENDER },
    { nullptr }
  };
  int option_index;
  int c;

  memset(&opt, '\0', sizeof opt);
  opt.parser = -1;

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
      if ((opt.parser = find_parser(optarg)) == -1) {
        fprintf(stderr, "no such parser: %s\n", optarg);
        list_parsers(stderr);
        opt.error = true;
      }
      break;
    case OPT_VERBOSE:
      opt.verbosity++;
      break;
    case OPT_CONFDIR:
      {
        struct config_dir *cd = calloc(1, sizeof *cd);
        if (cd == NULL) {
          fprintf(stderr, "could not allocate node: %s\n", strerror(errno));
          opt.error = true;
        } else {
          if (optarg[0] == '+' && !opt.confdirs) {
            cd->dir = optarg + 1;
            cd->next = get_defconf();
          } else {
            cd->dir = optarg;
            cd->next = opt.confdirs;
          }
          cd->node_needs_free = true;
          opt.confdirs = cd;
        }
      }
      break;
    case OPT_RENDER:
      for (opt.render_mode = 0;
           opt.render_mode < RENDER_MODE_MAX &&
             strcmp(optarg, render_mode_names[opt.render_mode]);
           opt.render_mode++);
      if (opt.render_mode == RENDER_MODE_MAX)
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

static void free_options(void) {
  struct config_dir **cdp, *cd;

  for (cdp = &opt.confdirs; (cd = *cdp);) {
    *cdp = cd->next;
    if (cd->name_needs_free)
      free(cd->dir);
    if (cd->node_needs_free)
      free(cd);
  }
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
  init_parsers();
  parse_options(argc, argv);

  if (opt.error) {
    usage(stderr);
    free_options();
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
    list_render_modes(stdout);
  }

  if (opt.version)
    version(stdout);

  if (opt.help || opt.version)
    goto finish;

  load_config(opt.confdirs ? opt.confdirs : get_defconf());

  if (opt.file) {
    rc = map_file(&input, max_buf, opt.file);
  } else {
    rc = map_stream(&input, max_buf, stdin);
  }

  if (rc != 0)
    return EXIT_FAILURE;

  /* Attempt to determine HTML type */
  if (opt.parser < 0) {
    parser_match(&input);
  }

  /* Choose default parser */
  if (opt.parser < 0) {
    opt.parser = 0;
  }

  rc = parser_defs[opt.parser]->parse_fn(&input);

  free_map(&input);
  free_parsers();

finish:
  free_options();
  return EXIT_SUCCESS;
}
