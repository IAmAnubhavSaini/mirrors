# Process this file with automake to produce Makefile.in

SUBDIRS = src doc misc contrib

# AUTOMAKE_OPTIONS = foreign
# 
# # We must run make the .dist_classes and various .dist_headers after
# # recompiling, but before making a dist, as they uses dependency information to
# # figure out which header files should be included in the dist.
# 
# DOTDISTS = src/.dist_classes src/.dist_headers
# 
# MAKEFILESAM = Makefile.am doc/Makefile.am src/Makefile.am \
# 	src/scm/Makefile.am
# 
# # FIXME: Use the BASH variable to execute the script. The problem is
# # that the script attempts to start itselt with "$0 args".
# $(MAKEFILESAM): $(addsuffix .in,$(MAKEFILESAM))
# 	(cd $(srcdir) && BASH=$(BASH) \
# 	    SELF="$(BASH) ./make_am" $(BASH) ./make_am)
# 
# $(DOTDISTS): depend
# 	(cd $(srcdir) && BASH=$(BASH) \
# 	    SELF="$(BASH) ./make_am" $(BASH) ./make_am --rethink)
# 
# .PHONY: dotdists
# dotdists: $(DOTDISTS)

.PHONY: bootstrap
bootstrap:
	(cd src && $(MAKE) bootstrap)

# MAINTAINERCLEANFILES = aclocal.m4 configure \
# 	config.status config.log config.cache

# Is this rule needed?
# all-recursive check-recursive: config.h

EXTRA_DIST = ANNOUNCE AUTHORS FAQ README COPYING \
	ChangeLog ChangeLog.1 ChangeLog.2 \
	NEWS FAQ distribution-key.gpg .bootstrap

