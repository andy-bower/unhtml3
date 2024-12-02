# unhtml 3.0+

The `unhtml` utility removes HTML markup from a document and outputs plain
text on stdout in the UTF-8 charset. The input needs to be UTF-8; if the
input uses another character set then this should be transformed first.

`unhtml` is an extractor not a renderer; other tools such as `w3m` are
more appropriate if the output is to be embellished with rendering based
on HTML markup.

Content within `<SCRIPT>` and `<STYLE>` elements is ignored.

# Relationship to unhtml 2.2, 2.3

This version of unhtml is a complete rewrite and drop-in replacement of unhtml
(originally known as 'clean') by Kevin Swan in 1998. The original program
appears to be abandoned upstream and is currently maintained in Debian as
unhtml at https://salsa.debian.org/debian/unhtml/-/tree/upstream?ref_type=heads

# Motivation

The current `unhtml` package in Debian has significant flaws which in my
view aren't sensible fixable by patching the old version. This new version
uses a proper HTML parser library (initially, libgumbo, for its ability to
understand HTML 5 tag soup) with complete capability to handle entities.

The major flaws of the old package are:

* Only understands and emits ISO-8859-1, not UTF-8.
* Has handling limitations with some constructs such as DOCTYPE, CDATA
  sections and STYLE elements.

The existing package has a non-trivial
[popcon count](https://qa.debian.org/popcon.php?package=unhtml) if I have
understood the metric correctly, so there evidently is some demand for a
standalone utility even though it would probably be simpler to run a command
like `xmllint -html -xpath //text()`. So I thought it would be a good idea
to come up with a drop-in replacement that is at least fit for purpose
for that constituency of users.

# Limitations

Numerous... I'll raise some as _issues_ in due course.

Essentially _if I wanted to go there I wouldn't start here_. See above for
rationale for existence!

* **Only** understands UTF-8
  - does not convert output to current locale
  - ignores alternative charset selected via `<META>` or `<?xml>`
    declarations.
* Has issues with some SGML content (see `unhtml(1)` manpage).
* This version is slower than the original and `xmllint` (but faster than `w3m`).

# Contributing

Contributions and criticisms welcome!

# Releasing and downstream packaging

It is suggested that downstream packagers use signed tags from this
repository as the canonical upstream form, rather than any tarball
artefacts. Releases will be signed by the following PGP key:

```
pub   rsa4096/0x4510339430FC9F34 2022-08-16 [C]
      Key fingerprint = 06AB 786E 936C 6C73 F6D8  130C 4510 3394 30FC 9F34
```

# Copyright and licence

This project shares nothing in common with its predecessor apart from
having a backwards compatible interface. It is licensed with the MIT
licence and Copyright (c) 2024, Andrew Bower <andrew@bower.uk>
