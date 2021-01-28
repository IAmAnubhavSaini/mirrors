# Process this file with automake to produce Makefile.in

SUBDIRS = argp rsync scm sftp spki . testsuite

include .dist_classes
include .dist_headers

BUILT_SOURCES = environ.h

SCHEME = $(SCHEME_PROGRAM) -l $(srcdir)/scm/$(SCHEME_NAME)-compat.scm

EXTRA_PROGRAMS = lsh-krb-checkpw lsh-pam-checkpw srp-gen

bin_PROGRAMS = lsh lshg lsh-keygen lsh-writekey lsh-decrypt-key \
	       lsh-decode-key lsh-export-key lsh-make-seed \
	       @SRP_PROGRAM@

sbin_PROGRAMS = lshd lsh-execuv @KRB_PROGRAM@ @PAM_PROGRAM@
bin_SCRIPTS = lsh-authorize ssh-conv lsh-upgrade lsh-upgrade-key lcp
noinst_LIBRARIES = liblsh.a

# Don't use BUILT_SOURCES, as all these files are #included, and can't be
# compiled or dependency checked independently.

generated_sources = atoms_defines.h atoms_gperf.c atoms_table.c \
	packet_types.h

.PHONY: bootstrap

# Not all potential .x files can be generated
bootstrap: $(generated_sources) $(dist_classes)
	(cd spki && $(MAKE) bootstrap)

liblsh_a_SOURCES = abstract_io.c abstract_crypto.c \
	algorithms.c alist.c atoms.c \
	channel.c channel_commands.c channel_forward.c \
	charset.c \
	client.c client_escape.c client_keyexchange.c \
	client_pty.c client_session.c client_userauth.c client_x11.c \
	combinators.c command.c compress.c \
	parse_config.c connection.c connection_commands.c \
	crypto.c \
	daemon.c \
	debug.c dh_exchange.c \
	dsa.c \
	exception.c \
	format.c gc.c \
	gateway.c gateway_channel.c gateway_commands.c \
	handshake.c \
	interact.c io.c io_commands.c \
	keyexchange.c \
	list.c lookup_verifier.c \
	lock_file.c \
	lsh_string.c parse.c \
	pkcs5.c \
	publickey_crypto.c queue.c \
	randomness.c \
	read_data.c read_file.c read_line.c read_packet.c \
	reaper.c resource.c \
	rsa.c \
	server.c server_authorization.c server_keyexchange.c \
	server_password.c server_publickey.c \
	server_pty.c server_session.c server_userauth.c \
	server_x11.c sexp.c socks.c \
	spki.c srp_exchange.c \
	string_buffer.c suspend.c \
	tcpforward.c tcpforward_commands.c \
	translate_signal.c tty.c \
	unix_interact.c unix_process.c unix_random.c unix_user.c \
	userauth.c \
	werror.c write_buffer.c write_packet.c \
	xalloc.c xauth.c zlib.c

liblsh_a_LIBADD = @LIBOBJS@

# config_parse.c: config_parse.y
# 	bison $< -o $@T
# 	test -s $@T && mv -f $@T $@

# FIXME: Won't work if the filename contains `,', `\', `"' or newline.
# NOTE: This rule is used by non-maintainer builds, so avoid using
# $< and $@ which are only available with GNU make.
environ.h: environ.h.in Makefile
	(echo '/* Automatically generated from environ.h.in, by make. */' ; \
	sed -e 's,@''prefix@,$(prefix),' \
	    -e 's,@''bindir@,$(bindir),' \
	    -e 's,@''sbindir@,$(sbindir),' \
	    -e 's,@''libexecdir@,$(libexecdir),' \
	    -e 's,@''sysconfdir@,$(sysconfdir),' \
	  < $(srcdir)/environ.h.in) >environ.hT
	test -s environ.hT && mv -f environ.hT environ.h

atoms_defines.h: atoms.in process_atoms
	$(srcdir)/process_atoms header <$< >$@T
	test -s $@T && mv -f $@T $@

# With the next version of gperf, add -F ", 0"
GPERF_FLAGS = -LANSI-C -t -c -C -l -k1,3,6,10,21,$$ -N gperf_atom 
atoms_gperf.c: atoms.in process_atoms
	$(srcdir)/process_atoms gperf <$< \
             | gperf $(GPERF_FLAGS) | sed 's/{""}/{"", 0}/g' >$@T 
	test -s $@T && mv -f $@T $@

atoms_table.c: atoms.in process_atoms
	$(srcdir)/process_atoms table <$< >$@T
	test -s $@T && mv -f $@T $@

packet_types.h: ssh.h invert-defs
	$(srcdir)/invert-defs <$< >$@T
	test -s $@T && mv -f $@T $@

lsh_krb_checkpw_LDADD=@KRB_LIBS@

lsh_execuv_LDADD=

LDADD = liblsh.a spki/libspki.a -lnettle -lhogweed @LIBARGP@

# To avoid having to link lshg with nettle, link with dummy.o.

# FIXME: We still link with gmp and zlib, to avoid that we'd need to
# remove them from LIBS. The only libraries really needed for lshg is
# liboop and perhaps libXau.

lshg_SOURCES = lshg.c dummy.c
lshg_LDADD = liblsh.a @LIBARGP@

class-map: $(srcdir)/make-class-map $(dist_classes)
	cat $(dist_classes) | grep '^CLASS:' \
	| sort | awk -f $(srcdir)/make-class-map > $@

# Is this rule needed?
# all-recursive check-recursive: config.h

EXTRA_DIST = lsh-authorize ssh-conv lcp \
	lsh-upgrade lsh-upgrade-key \
	atoms.in process_atoms \
	environ.h.in \
	invert-defs \
	$(generated_sources) \
	sexp_chars.in \
	jpoll.h jpoll.c \
	$(dist_headers) $(dist_classes)

CLEANFILES = *.hT *.cT *.xT
DISTCLEANFILES = environ.h
MAINTAINERCLEANFILES = $(dist_classes) $(DEP_FILES)

# Class files
# SUFFIXES = .xh .xc

.PHONY: gcov-list
gcov-list:
	for f in *.c; do gcov "$$f"; done
	grep -c '##' *.gcov | grep -v ':0$$' | sort -t : -k 2 -nr >gcov-list

.PHONY: depend
depend:
	rm -f $(DEP_FILES)
	make $(DEP_FILES)

# This is GNU make specific

%.x: % $(srcdir)/scm/gaba.scm $(srcdir)/scm/compiler.scm
	$(SCHEME) -e main -l $(srcdir)/scm/compiler.scm \
		  -s $(srcdir)/scm/gaba.scm <$< >$@T
	test -s $@T && mv -f $@T $@

ETAGS_ARGS = -r '/     (name[ \t]+\([a-zA-Z0-9_]+\)/\1/' \
             -r '/DEFINE[^(]*(\([a-zA-Z0-9_]+\)/\1/'

