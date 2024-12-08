/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk> */

#include "config.h"
#include "render.h"

enum render_state {
  STATE_NEWLINE,
  STATE_NEWLINE2,
  STATE_NEWLINE3PLUS,
  STATE_SPACE,
  STATE_SPACE2PLUS,
  STATE_TEXT,
};

enum render_state state = STATE_NEWLINE;

void render_element(const char8_t *tag, bool end, const struct render_elem *rendering) {
  if (opt.render_mode == RENDER_MODE_LITERAL)
    return;

  if (!rendering)
    rendering = get_rendering(tag);

  if (rendering) {
    switch (rendering->spacing) {
    case SPACING_NONE:
      break;
    case SPACING_PARA:
      puts("");
      break;
    case SPACING_NEWLINE:
      if (!end)
        puts("");
      break;
    case SPACING_SPACE:
      if (!end)
        putchar(' ');
      break;
    }
  }
}

void render_text(const char8_t *text) {
  fputs((char *) text, stdout);
}
