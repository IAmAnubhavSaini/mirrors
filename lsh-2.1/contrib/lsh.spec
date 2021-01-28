Summary:        lsh - secure connections with secsh2 protocol
Name:           lsh
Version:        2.1
Release:        1
Copyright:      GPL
Group:          Application/Internet
Source0:        ftp://ftp.lysator.liu.se/pub/security/lsh/%{name}-%{version}.tar.gz 
BuildRoot:      /var/tmp/%{name}-%{version}-root
Prefix:         /usr
Packager:       Thayne Harbaugh <thayne@plug.org>
URL:            http://www.net.lut.ac.uk/psst/
Requires:       chkconfig
Requires:	info


%description 
lsh implements the secsh2 protocol


%prep
%setup


%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%prefix
if [ "$SMP" != "" ]; then
  (make "MAKE=gmake -k -j $SMP"; exit 0)
fi
gmake


%install
rm -rf $RPM_BUILD_ROOT

gmake prefix=$RPM_BUILD_ROOT%{prefix} install

install -d -m 0755 $RPM_BUILD_ROOT/etc/rc.d/init.d

install -m 0755 contrib/lshd.rhlinux.init \
        $RPM_BUILD_ROOT/etc/rc.d/init.d/lshd

# ugly, but it's also ugly to list all files in bin
# make install should strip - not here
strip $RPM_BUILD_ROOT%{prefix}/bin/* || true
strip $RPM_BUILD_ROOT%{prefix}/sbin/*

gzip $RPM_BUILD_ROOT%{prefix}/man/*/*

rm -f doc/Makefile*


%clean
rm -rf $RPM_BUILD_ROOT


%post
chkconfig --add lshd
if [ ! -e /etc/lsh_host_key -o ! -e /etc/lsh_host_key.pub ]
then
        rm -f /etc/lsh_host_key*
        /usr/bin/lsh-keygen -l 8 | /usr/bin/lsh-writekey -o /etc/lsh_host_key
fi
/sbin/install-info --info-dir=%{prefix}/info %{prefix}/info/lsh.info


%preun
if [ "$1" -eq 0 ]
then
        chkconfig --del lshd || exit 0
	/sbin/install-info --delete --info-dir=%{prefix}/info %{prefix}/info/lsh.info
fi


%files 
%defattr(-, root, root)

%doc AUTHORS COPYING ChangeLog FAQ NEWS README
%doc doc

%config/etc/rc.d/init.d/lshd
%{prefix}/bin/*
%{prefix}/man/*/*
%{prefix}/info/lsh*
%{prefix}/sbin/*


%changelog

* Wed Jun 28 2000 Thayne Harbaugh <thayne@plug.org>
- ripped out man install - make install now does it
- there is now an lsh.info that is handled
- various simplifications
- spelling error

* Thu Jan 06 2000 Thayne Harbaugh <thayne@northsky.com>
- lshd.rhlinux.init is now in contrib dir - removed Source1
- fixed preun $1 comparision - was [ "$1" -eq 1 ]

* Thu Sep 28 1999 Thayne Harbaugh <thayne@northsky.com>
- first rpm

