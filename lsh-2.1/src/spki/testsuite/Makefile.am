AM_CPPFLAGS = -I$(top_srcdir)

TS_PROGS = principal-test date-test tag-test read-acl-test \
	   lookup-acl-test read-cert-test cdsa-reduce-test

TS_SH = check-signature-test make-signature-test \
	delegate-test reduce-test

TS_ALL = $(TS_PROGS) $(TS_SH)

noinst_PROGRAMS = $(TS_PROGS)

LDADD = testutils.o ../libspki.a -lhogweed -lnettle

include .dist_cdsa

EXTRA_DIST = testutils.c testutils.h $(TS_SH) \
	run-tests setup-env $(dist_cdsa) key-1 key-1.pub key-2 key-2.pub

CLEANFILES = test.in test.out test.acl test.canonical

# Files and directories used by the test cases.
distclean-local:
	rm -rf testhome

all:
	@echo make check to build testprograms.

.PHONY: check

check: $(TS_ALL)
	LD_LIBRARY_PATH=../../nettle/.lib srcdir=$(srcdir) \
		$(srcdir)/run-tests $(TS_ALL)

