noinst_PROGRAMS = spki-check-signature spki-make-signature \
	spki-delegate spki-reduce

AM_CPPFLAGS = -I$(top_srcdir)

# libnettle.a and libspki.a are added at the end to make sure all
# programs depend on it. It seems there's no DEPENDENCIES variable
# that affects all programs.

LDADD = misc.o getopt.o getopt1.o \
	../libspki.a -lhogweed -lnettle

spki_make_signature_SOURCES = spki-make-signature.c sign.c
spki_delegate_SOURCES = spki-delegate.c sign.c

EXTRA_DIST = misc.c misc.h sign.h getopt.h getopt.c getopt1.c
