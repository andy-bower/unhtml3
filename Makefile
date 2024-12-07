# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk>

VERSION ?= 3.0.0
prefix ?= /usr/local
CFLAGS ?= -g -O2
CFLAGS += -MMD -MP \
	  -Wall -Wimplicit-fallthrough -Werror \
	  -std=c23 \
	  -D_GNU_SOURCE \
	  -DUNHTML_VERSION=$(VERSION) -DINST_PREFIX=$(prefix)\
	  -I/usr/include/libxml2
LDLIBS = -lgumbo -lxml2
LOOSE_DIFF = diff -u --ignore-space-change --ignore-blank-lines
INSTALL = install
DEP = $(wildcard *.d)
prefix ?= /usr
name := unhtml
testfiles := testfiles/

OBJS = unhtml.o load.o config.o render.o

ifndef NO_GUMBO
CFLAGS += -DWITH_GUMBO
LDLIBS += -lgumbo
OBJS += parse-gumbo.o
endif

ifndef NO_LIBXML2
CFLAGS += -DWITH_LIBXML2 -I/usr/include/libxml2
LDLIBS += -lxml2
OBJS += parse-libxml2.o
endif

.PHONY: all clean install

all: $(name)

-include $(DEP)

$(name): $(OBJS)

clean:
	$(RM) $(name) $(OBJS) $(DEP)

install:
	$(INSTALL) -m 755 -D -t $(DESTDIR)$(prefix)/bin            $(name)
	$(INSTALL) -m 644 -D -t $(DESTDIR)$(prefix)/share/man/man1 $(name).1
	$(INSTALL) -m 644 -D -t $(DESTDIR)$(prefix)/share/$(name)  $(wildcard default/*.xml)

include test.mk
