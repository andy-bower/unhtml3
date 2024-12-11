# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: (c) Copyright 2024 Andrew Bower <andrew@bower.uk>

# Command to unvoke unhtml as if it were installed
TEST_INVOKE_UNHTML=$(name) -confdir default

.PHONY: check debug clean-tests check-testfiles check-xml

check: check-xml check-testfiles

debug: LOOSE_DIFF:=diff -u
debug: check

check-xml:
	xmllint -noout $(wildcard default/*.xml)

# Test suite follows pattern from predecessor unhtml-2.3.9:
#   <https://salsa.debian.org/debian/unhtml/-/blob/upstream/2.3.9/tests/Makefile?ref_type=tags>
# Rewritten so you just drop a matching .html and .out pair into testfiles/
#
# The 'check' target tolerates differences in amount of whitespace.
# The 'debug' target shows any difference at all.

$(testfiles)%.tmp: $(testfiles)%.html $(name)
	./$(TEST_INVOKE_UNHTML) $< > $@

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
