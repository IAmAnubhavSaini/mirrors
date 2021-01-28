# Testsuite

# -O0 is not recogniced on AIX
# AM_CFLAGS = -O0

AM_CPPFLAGS = -I$(srcdir)/.. -I..

TS_PROGS = arcfour-test aes-test blowfish-test cast128-test \
	   des-test \
	   serpent-test twofish-test \
           md5-test sha1-test rsa-test dsa-test spki-tag-test \
           string-test parse-config-test sockaddr2info-test \
	   utf8-test

TS_SH = conv-1-test conv-2-test conv-3-test \
	export-1-test keygen-1-test keygen-2-test \
	write-key-1-test write-key-2-test seed-test \
	lsh-1-test lsh-2-test lsh-3-test lsh-4-test \
	lsh-5-test lsh-6-test lsh-7-test lsh-8-test lsh-9-test \
	lsh-encrypted-key-test lsh-decrypt-key-test \
	lsh-cat-test lshg-1-test lshg-cat-test lshg-cat-2-test \
	daemon-fd-test \
	tcpip-local-test tcpip-local-in-use-test tcpip-remote-test \
	socks5-test socks4-test \
	lshg-tcpip-local-test \
	lcp-test \
	rapid7-lsh-test rapid7-lshd-test login-auth-test lshd-no-auth-test \
	lshd-random-input-test

TS_ALL = $(TS_PROGS) $(TS_SH)

noinst_PROGRAMS = $(TS_PROGS) mini-inetd tcpconnect

# Workaround to get automake to keep dependencies for testutils.o
EXTRA_PROGRAMS = testutils

LDADD = testutils.o ../liblsh.a ../spki/libspki.a -lnettle -lhogweed \
	$(DOTDOT_LIBARGP)
mini_inetd_LDADD = 
tcpconnect_LDADD = 

include .dist_rapid7

EXTRA_DIST = testutils.h $(TS_SH) \
	functions.sh run-tests setup-env key-1.private key-2.private \
	$(dist_rapid7) UTF-8-test.txt

# FIXME: Move into some tmp directory.
CLEANFILES = foo foo.pub test-seed-1 test-seed-2 test.out1 test.out2 lcp-file-*.txt

# Files and directories used by the test cases.
distclean-local:
	rm -rf temphome home *.pid

DISTCLEANFILES = yarrow-seed-file

all:
	@echo make check to build testprograms.

.PHONY: check-some check-more

# sexp-conv may be dynamically linked
check: $(TS_ALL)
	srcdir=$(srcdir) \
		$(srcdir)/run-tests $(TS_ALL)

