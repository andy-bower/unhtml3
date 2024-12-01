# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk>

VERSION ?= 3.0.0
CFLAGS ?= -g
CFLAGS += -Wall -Wimplicit-fallthrough -Werror -std=c23 -D_GNU_SOURCE -DUNHTML_VERSION=$(VERSION)
LDLIBS = -lgumbo
LOOSE_DIFF = diff -u --ignore-space-change --ignore-blank-lines
INSTALL = install

prefix ?= /usr

name := unhtml
testfiles := testfiles/

.PHONY: all clean install check debug clean-tests check-testfiles

all: $(name)

$(name): unhtml.o

clean:
	$(RM) $(name) unhtml.o

install:
	$(INSTALL) -m 755 -D $(name) $(DESTDIR)$(prefix)/bin/$(name)
	$(INSTALL) -m 644 -D $(name).1 $(DESTDIR)$(prefix)/share/man/man1/$(name).1

# Test suite follows pattern from predecessor unhtml-2.3.9:
#   <https://salsa.debian.org/debian/unhtml/-/blob/upstream/2.3.9/tests/Makefile?ref_type=tags>
# Rewritten so you just drop a matching .html and .out pair into testfiles/
#
# The 'check' target tolerates differences in amount of whitespace.
# The 'debug' target shows any difference at all.

$(testfiles)%.tmp: $(testfiles)%.html $(name)
	@./$(name) $< > $@

$(testfiles)%.result: $(testfiles)%.out $(testfiles)%.tmp
	@$(LOOSE_DIFF) $^ && echo $(patsubst %.result,%,$@) > $@ || truncate -s 0 $@

clean-tests:
	$(RM) $(testfiles)result $(testfiles)*.tmp

check-testfiles: results=$(filter %.result,$^)
check-testfiles: tests=$(patsubst %.result,%,$(results))
check-testfiles: clean-tests $(name) $(patsubst %.html,%.result,$(wildcard $(testfiles)*.html))
	@$(if $(filter-out $(foreach r,$(results),$(file <$r)),$(tests)), \
  echo "At least one test failed"; false,true) && a=$$?; \
	$(RM) $(results) && return $a

check: check-testfiles

debug: LOOSE_DIFF:=diff -u
debug: check
