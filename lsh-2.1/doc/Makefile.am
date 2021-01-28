# Process this file with automake to produce Makefile.in

man_MANS=DSA.5 SHA.5 SPKI.5 secsh.5 \
	 lsh.1 lsh-authorize.1 lsh-decode-key.1 \
	lsh-decrypt-key.1 lsh-export-key.1 lshg.1 \
	lsh-keygen.1 lsh-make-seed.1 lsh-upgrade.1 \
	lsh-upgrade-key.1 lsh-writekey.1 ssh-conv.1 \
	lshd.8


# MANPAGES = $(wildcard *.[1-9])
info_TEXINFOS = lsh.texinfo

EXTRA_DIST = HACKING NOTES PORTS TASKLIST \
	configuration.txt gateway-mode.txt \
	lsh.html $(man_MANS) \
	srp-spec.nroff srp-spec.txt fixff.sh fixff.pl

# DVIS = $(addsuffix .dvi, $(man_MANS))
# PSS =  $(addsuffix  .ps, $(man_MANS))

# all:	dvi ps

# dvi:	$(DVIS)
# 
# ps:	$(PSS)

all-local: srp-spec.txt lsh.html 

# %.dvi: %
# 	$(GROFF) -Tdvi -mandoc $< > $@
# 
# %.ps: %
# 	$(GROFF) -Tps -mandoc $< > $@
# 
# 

%.txt : %.nroff
	$(GROFF) -ms -Tlatin1 $< | perl $(srcdir)/fixff.pl >$@T
	test -s $@T && mv -f $@T $@

%.html : %.texinfo
	(cd $(srcdir) \
	 && $(MAKEINFO) --html --no-split --output $@T $(<F) \
	 ; test -s $@T && mv -f $@T $@)
