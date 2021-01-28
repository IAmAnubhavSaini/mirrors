SUBDIRS = . tools testsuite

noinst_LIBRARIES = libspki.a
# libspkiincludedir = $(includedir)/nettle

libspki_a_SOURCES = certificate.c parse.c parse-transport.c names.c \
	tag.c reduce.c \
	verify.c \
	certificate.h parse.h tag.h

# Does this get $(srcdir) right?
spki-types.h: spki-types.in process-types
	$(srcdir)/process-types header < $< > $(srcdir)/spki-types.hT
	test -s $(srcdir)/spki-types.hT \
		&& mv -f $(srcdir)/spki-types.hT $(srcdir)/spki-types.h

spki-type-names.h: spki-types.in process-types
	$(srcdir)/process-types names < $< > $(srcdir)/spki-type-names.hT
	test -s $(srcdir)/spki-type-names.hT \
		&& mv -f $(srcdir)/spki-type-names.hT $(srcdir)/spki-type-names.h

GPERF_FLAGS = -LANSI-C -t -c -C -l -E -o -k1,$$ -N spki_gperf

spki-gperf.h: spki-types.in process-types
	$(srcdir)/process-types gperf < $< | tee spki-gperf.in \
		| gperf $(GPERF_FLAGS) | sed 's/{""}/{"", 0}/g' \
		> $(srcdir)/spki-gperf.hT
	test -s $(srcdir)/spki-gperf.hT \
		&& mv -f $(srcdir)/spki-gperf.hT $(srcdir)/spki-gperf.h

EXTRA_DIST = process-types \
	spki-types.in spki-types.h spki-gperf.h spki-type-names.h

.PHONY: bootstrap
bootstrap: spki-types.h spki-gperf.h spki-type-names.h

# Is this right?
# .PHONY: $(srcdir)/spki-types.h
# .PHONY: $(srcdir)/spki-gperf.h
# .PHONY: $(srcdir)/spki-type-names.h
# $(srcdir)/% : %
