dnl Process this file with autoconf to produce a configure script.
AC_INIT([lsh], [2.1], [bug-lsh@gnu.org]) 
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR([src/lsh.c])
# Needed to stop autoconf from looking for files in parent directories.
AC_CONFIG_AUX_DIR([.])
	
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([config.h])

# We want an absolute path to the source-dir.
case "$srcdir" in
    /*)
    ;;
    *)
	oldsrcdir="$srcdir"
	srcdir="`cd \"$srcdir\";pwd`"
	AC_MSG_WARN([ Converted $oldsrcdir to $srcdir,
If this does not work, please use an absolute path to the configure script. ])
    ;;
esac

# src/rsync relies on lsh's definitions of WRITE_UINT32 etc.
# CPPFLAGS="$CPPFLAGS -I$srcdir/src -DLSH"

# GNU libc defaults to supplying the ISO C library functions only. 
# initgroups() and strsignal() are extensions; the _GNU_SOURCE define
# enables these extensions.
# Enable it on all systems; no problems have been reported with it so far.

AC_GNU_SOURCE

# Check options
AC_ARG_ENABLE(debug_alloc,
  AC_HELP_STRING([--enable-debug-alloc], [Enable memory allocation sanity checks]),,
  [enable_debug_alloc=no])

AH_TEMPLATE([DEBUG_ALLOC],
	    [Define to enable sanity checking on memory allocation and casting])
if test x$enable_debug_alloc = xyes ; then
  AC_DEFINE(DEBUG_ALLOC)
fi

AC_ARG_ENABLE(debug_trace,
  AC_HELP_STRING([--enable-debug-trace], [Enable tracing support]),,
  [enable_debug_trace=no])

AH_TEMPLATE([DEBUG_TRACE], [Define to enable tracing])
if test x$enable_debug_trace = xyes ; then
  AC_DEFINE(DEBUG_TRACE)
fi

AC_ARG_ENABLE(gcov,
  AC_HELP_STRING([--enable-gcov], [Instrument for gcov (requires a modern gcc)]),,
  [enable_gcov=no])

AC_ARG_ENABLE(profiling,
  AC_HELP_STRING([--enable-profiling], [Instrument for gprof profiling]),,
  [enable_profiling=no])


AC_ARG_WITH(zlib,
  AC_HELP_STRING([--without-zlib], [Don't use zlib compression]),,
  [with_zlib=yes])

AC_ARG_WITH(tcpwrappers,
  AC_HELP_STRING([--with-tcpwrappers], 
                [Use tcp-wrappers for filtering connections]),
  [if test -z "$withval"; then 
    with_tcpwrappers=yes; 
  else 
    with_tcpwrappers="$withval";
  fi],
  [with_tcpwrappers=no])

# Checking this variable is delayed until we have checked if zlib is
# actually available.

AC_ARG_ENABLE(pty,
  AC_HELP_STRING([--without-pty], [Disable pty support]),,
  [enable_pty=yes])

AH_TEMPLATE([WITH_PTY_SUPPORT], [Define to enable pty support])
if test x$enable_pty = xyes ; then
  AC_DEFINE(WITH_PTY_SUPPORT)
fi

AC_ARG_ENABLE(srp,
  AC_HELP_STRING([--disable-srp], [Disable the (experimental) support for SRP]),,
  [enable_srp=yes])

AC_ARG_ENABLE(gss,
  AC_HELP_STRING([--disable-gss], [Disable the (experimental) support for GSS]),,
  [enable_gss=yes])

AC_ARG_ENABLE(kerberos,
  AC_HELP_STRING([--disable-kerberos], [Don't support kerberos]),,
  [enable_kerberos=yes])

AC_ARG_ENABLE(pam,
  AC_HELP_STRING([--disable-pam], [Don't support PAM]),,
  [enable_pam=yes])

AH_TEMPLATE([WITH_SRP], [Define if SRP should be supported])
if test x$enable_srp = xyes ; then
  AC_DEFINE(WITH_SRP)
  SRP_PROGRAM=srp-gen
fi

AC_SUBST(SRP_PROGRAM)

AC_ARG_ENABLE(initgroups_workaround,
  AC_HELP_STRING([--enable-initgroups-workaround],
		 [Use a special initgroups for supporting more groups]),,
  [enable_initgroups_workaround=no])

AH_TEMPLATE([INITGROUPS_WORKAROUND],
	    [Define to enable the initgroups workaround])
if test x$enable_initgroups_workaround = xyes ; then
  AC_DEFINE(INITGROUPS_WORKAROUND)
fi


AC_ARG_ENABLE(tcp_forward,
  AC_HELP_STRING([--disable-tcp-forward], [Disable tcp forwarding]),,
  [enable_tcp_forward=yes])

AH_TEMPLATE([WITH_TCP_FORWARD], [Define to enable tcp forwarding])
if test x$enable_tcp_forward = xyes ; then
  AC_DEFINE(WITH_TCP_FORWARD)
fi

AC_ARG_ENABLE(x11_forward,
  AC_HELP_STRING([--disable-x11-forward], [Disable x11 forwarding (proxy only)]),,
  [enable_x11_forward=yes])

AH_TEMPLATE([WITH_X11_FORWARD], [Define to enable x11 forwarding])
if test x$enable_x11_forward = xyes ; then
  AC_DEFINE(WITH_X11_FORWARD)
fi

AC_ARG_ENABLE(agent_forward,
  AC_HELP_STRING([--disable-agent-forward], [Disable auth-agent forwarding (proxy only)]),,
  [with_agent_forward=yes])

AH_TEMPLATE([WITH_AGENT_FORWARD],
	    [Define to enable authentication agent forwarding])
if test x$disable_agent_forward = xyes ; then
  AC_DEFINE(WITH_AGENT_FORWARD)
fi

AC_ARG_WITH(scheme,
  AC_HELP_STRING([[--with-scheme[=PROGRAM]]], [Use a particular scheme implementation]),,
  [with_scheme=])

AC_ARG_WITH(system-argp,
  AC_HELP_STRING([--without-system-argp], [Don't use the system's argp]),,
  [with_system_argp=yes])

# IPv6 support
AC_ARG_ENABLE(ipv6,
  AC_HELP_STRING([--disable-ipv6], [Disable IPv6 support]),,
  [enable_ipv6=yes])

# utmp/wtmp logging 
AC_ARG_ENABLE(utmp,
  AC_HELP_STRING([--disable-utmp], [Disable utmp and wtmp support]),,
  [enable_utmp=yes])

AH_TEMPLATE([WITH_UTMP], [For utmp support])
if test x$enable_utmp = xyes ; then
  AC_DEFINE(WITH_UTMP)
fi

AC_ARG_WITH(include-path,
  AC_HELP_STRING([--with-include-path], [A colon-separated list of directories to search for include files]),,
  [with_include_path=''])

if test x$with_include_path != x ; then
  CPPFLAGS="$CPPFLAGS -I`echo $with_include_path | sed 's/:/ -I/g'`"
fi

AC_ARG_WITH(lib-path,
  AC_HELP_STRING([--with-lib-path], [A colon-separated list of directories to search for libraries]),,
  [with_lib_path=''])

if test x$with_lib_path != x ; then
  LDFLAGS="$LDFLAGS -L`echo $with_lib_path | sed 's/:/ -L/g'`"
fi

LSH_RPATH_INIT([`echo $with_lib_path | sed 's/:/ /g'` \
    `echo $exec_prefix | sed "s@^NONE@$prefix/lib@g" | sed "s@^NONE@$ac_default_prefix/lib@g"` \
    /usr/local/lib /sw/local/lib /sw/lib \
    /usr/gnu/lib /opt/gnu/lib /sw/gnu/lib /usr/freeware/lib /usr/pkg/lib])

dnl echo RPATH_CANDIDATE_DIRS = $RPATH_CANDIDATE_DIRS

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_RANLIB

if test "x$ac_cv_prog_cc_stdc" = xno ; then
  AC_MSG_ERROR([the C compiler doesn't handle ANSI-C])
fi

# Use a particular scheme implementation?
if test x$with_scheme != x ; then
  # If an absolute path is given, don't AC_PATH_PROG but just use it
  if grep "^/" > /dev/null <<EOF
$with_scheme 
EOF  
  then
    SCHEME_PROGRAM="$with_scheme" 
  else
    AC_PATH_PROG(SCHEME_PROGRAM, $with_scheme,, $PATH)
  fi
else  
  AC_PATH_PROGS(SCHEME_PROGRAM, guile scsh,, $PATH)
fi

if test "x$SCHEME_PROGRAM" = x ; then
  AC_MSG_WARN([No scheme implementation found.
This is fine as long as you don't modify the source files.])
  SCHEME_PROGRAM=false
fi

SCHEME_NAME=`basename $SCHEME_PROGRAM`
AC_SUBST(SCHEME_NAME)

if test "x$GROFF" = x; then
  AC_PATH_PROG(GROFF, groff,, $PATH)
fi
AC_ARG_VAR(GROFF, groff)

if test "x$M4" = x; then
  AC_PATH_PROG(M4, m4, m4)
fi
AC_ARG_VAR(M4, M4 macro processor)


# Checks for header files.
AC_HEADER_STDC

# We don't look in gmp2 anymore, as we need gmp-3.1 or better.
AC_CHECK_HEADERS(gmp.h,,
  AC_MSG_ERROR([gmp.h not found]))
  
AC_CHECK_HEADERS(fcntl.h strings.h sys/time.h unistd.h)
AC_CHECK_HEADERS(shadow.h)

if test x$with_zlib = xyes; then
  AC_CHECK_HEADERS(zlib.h,, [with_zlib=no])
fi
AC_CHECK_HEADERS(poll.h sys/poll.h)
AC_CHECK_HEADERS(libutil.h)
AC_CHECK_HEADERS(crypt.h)
AC_CHECK_HEADERS(pty.h)
AC_CHECK_HEADERS(stropts.h)
AC_CHECK_HEADERS(syslog.h)
AC_CHECK_HEADERS(argp.h)
AC_CHECK_HEADERS(sys/resource.h)

AC_CHECK_HEADERS(utmp.h, [have_utmp_h=yes], [have_utmp_h=no])
AC_CHECK_HEADERS(utmpx.h, [have_utmpx_h=yes], [have_utmpx_h=no])

dnl With current autoconfs we can't check for struct ut_tv with
dnl AC_CHECK_MEMBERS, but we can check for sub members. Leave the
dnl ut_tv check for clarity.

if test x$have_utmp_h = xyes; then
  AC_CHECK_MEMBERS([struct utmp.ut_host,
		    struct utmp.ut_name,
		    struct utmp.ut_user,
		    struct utmp.ut_time,
		    struct utmp.ut_tv,
		    struct utmp.ut_tv.tv_sec,
		    struct utmp.ut_id,
		    struct utmp.ut_syslen,
		    struct utmp.ut_session,
		    struct utmp.ut_pid,
		    struct utmp.ut_exit,
		    struct utmp.ut_exit.e_termination,
		    struct utmp.ut_exit.__e_termination,
		    struct utmp.ut_exit.ut_termination,
		    struct utmp.ut_addr,
		    struct utmp.ut_addr_v6],,,
[# if HAVE_UTMP_H
#  include <utmp.h>
# endif
])
fi

if test x$have_utmpx_h = xyes; then
  AC_CHECK_MEMBERS([struct utmpx.ut_host,
		    struct utmpx.ut_name,
		    struct utmpx.ut_user,
		    struct utmpx.ut_time,
		    struct utmpx.ut_id,
		    struct utmpx.ut_tv,
		    struct utmpx.ut_tv.tv_sec,
		    struct utmpx.ut_syslen,
		    struct utmpx.ut_session,
		    struct utmpx.ut_pid,
		    struct utmpx.ut_exit,
		    struct utmpx.ut_exit.e_termination,
		    struct utmpx.ut_exit.__e_termination,
		    struct utmpx.ut_exit.ut_termination,
		    struct utmpx.ut_addr,
		    struct utmpx.ut_addr_v6],,,
[# if HAVE_UTMPX_H
#  include <utmpx.h>
# endif
])
fi

AH_TEMPLATE([WITH_GSS_K5], [Whether to use gss K5 authorization (Heimdal/MIT)])
if test x$enable_gss != xno; then
  if test x$enable_gss = xk5; then
    AC_CHECK_PROG(KRB5CONFIG, krb5-config, krb5-config, no)
    if test x$KRB5CONFIG != xno; then
      CPPFLAGS="$CPPFLAGS `$KRB5CONFIG --cflags gssapi`"
      LIBS="$LIBS `$KRB5CONFIG --libs gssapi`"
      AC_CHECK_HEADERS([gssapi.h gssapi/gssapi.h gssapi/gssapi_generic.h])
      AC_DEFINE(WITH_GSS_K5)
      AC_CHECK_DECL(GSS_C_NT_HOSTBASED_SERVICE,, [
	AC_DEFINE(GSS_C_NT_HOSTBASED_SERVICE,
		gss_nt_service_name,
		[Work around buggy MIT library])], [
#ifdef HAVE_GSSAPI_H
#include <gssapi.h>
#endif
#ifdef HAVE_GSSAPI_GSSAPI_H
#include <gssapi/gssapi.h>
#endif
#ifdef HAVE_GSSAPI_GSSAPI_GENERIC_H
#include <gssapi/gssapi_generic.h>
#endif
])
    fi
  else
    AC_CHECK_HEADERS(gss.h,, [enable_gss=no])
    AC_CHECK_LIB(gss, gss_check_version,, [enable_gss=no])
  fi
fi

if test x$enable_kerberos = xyes; then
  AC_CHECK_HEADERS(krb5.h,, [enable_kerberos=no])
fi

if test x$enable_pam = xyes; then
  AC_CHECK_HEADERS(security/pam_appl.h,, [enable_pam=no])
fi

# Some systems (in particular, Unixware) doesn't have socklen_t, but
# uses size_t. Falling back to int will cause some warnings.

# AC_CHECK_TYPE doesn't work, probably because the type in
# question is not defined in sys/types.h.

LSH_TYPE_SOCKLEN_T

# Checks for libraries

AC_CHECK_LIB(gmp, __gmpz_getlimbn,,
    [AC_MSG_ERROR(
    [GNU MP not found, or not 3.1 or up, see http://gmplib.org])])

LSH_RPATH_FIX

AC_CHECK_LIB([oop], [oop_sys_new],,
  [AC_MSG_ERROR([liboop is missing. Get liboop from http://www.lysator.liu.se/liboop])])

LSH_RPATH_FIX

if test x$with_zlib = xyes; then
  AC_CHECK_LIB(z, inflate,, [with_zlib=no])
fi

if test x$with_zlib = xyes; then
  LSH_RPATH_FIX
fi

# FIXME: Should we let people use --with-tcpwrappers=/here/they/are?

if test x$with_tcpwrappers = xyes; then
  AC_CHECK_LIB(wrap, request_init,, 
  [
      AC_MSG_WARN([Failed to link libwrap, you might try adding -lsocket to LDFLAGS])
      with_tcpwrappers=no
  ])

  AC_CHECK_HEADER(tcpd.h,, [with_tcpwrappers=no])

  if test x$with_tcpwrappers = xno; then
    AC_MSG_WARN([Failed to find a working tcpwrappers setup, disabling wrappers])
  fi
fi

if test x$with_tcpwrappers = xyes; then
  LSH_RPATH_FIX
  AC_DEFINE(WITH_TCPWRAPPERS,1,[Whatever to use tcpwrappers])
else
  AC_DEFINE(WITH_TCPWRAPPERS,0,[Whatever to use tcpwrappers])
fi


# X11 stuff. We only need libXau.
# FIXME: Use AC_PATH_XTRA instead? 
AC_PATH_X

if test -z "$no_x" ; then
  if test -n "$x_includes" ; then
    CPPFLAGS="$CPPFLAGS -I$x_includes"
  fi
  if test -n "$x_libraries" ; then
    LDFLAGS="$LDFLAGS -L$x_libraries"
    LSH_RPATH_ADD($x_libraries)
  fi
fi

AC_CHECK_HEADERS(X11/Xauth.h)
AC_CHECK_LIB(Xau, XauGetAuthByAddr)
LSH_RPATH_FIX

AC_ARG_VAR(XAUTH_PROGRAM, Program used to handle .Xauthority-files by lshd)

if test -z "$XAUTH_PROGRAM"; then
  AC_PATH_PROG(XAUTH_PROGRAM, xauth,, /usr/X11R6/bin:/usr/openwin/bin:/usr/X/bin:/usr/X11/bin:/usr/bin:/usr/bin/X:/usr/bin/X11)
fi

if test -z "$XAUTH_PROGRAM"; then
  AC_MSG_WARN(Couldn't find xauth, try passing XAUTH_PROGRAM=/path/to/xauth to configure)
else
  AC_DEFINE_UNQUOTED(XAUTH_PROGRAM, "$XAUTH_PROGRAM", Program used by lshd to handle .Xauthority-files)
fi

# Move these checks for system libraries earlier, before checks for
# gmp, liboop etc

# glibc has a nis-library "libnsl", which we don't need. So use
# AC_SEARCH_LIBS rather than AC_CHECK_LIB.
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(inet_ntop, nsl)
AC_CHECK_LIB(socket, setsockopt)

AC_CHECK_LIB(crypt, crypt)
AC_CHECK_LIB(xnet, inet_addr)
# logwtmp is also in libutil
AC_CHECK_LIB(util, openpty)

# This macro is new in autoconf-2.13
AC_SEARCH_LIBS(syslog, bsd socket inet, [AC_DEFINE(HAVE_SYSLOG)])

AH_TEMPLATE([WITH_ZLIB], [Define if zlib should be used])
# Should we use zlib?
if test x$with_zlib = xyes ; then
  AC_DEFINE(WITH_ZLIB)
fi

AH_TEMPLATE([WITH_GSS], [Define if gss should be used])
# Should we use gss?
if test x$enable_gss != xno ; then
  AC_DEFINE(WITH_GSS)
fi

# The kerberos libraries are needed only to support the
# krb-check-passwd program, so we put them $KRB_LIBS, not in the
# ordinary $LIBS.

if test x$enable_kerberos = xyes; then
  LSH_CHECK_KRB_LIB(roken, strlcpy)
  LSH_CHECK_KRB_LIB(resolv, dn_expand)
  LSH_CHECK_KRB_LIB(des, des_cbc_encrypt)
  LSH_CHECK_KRB_LIB(asn1, der_get_octet_string)
  # Check for krb5_cc_gen_new too?
  # krb5_verify_user_lrealm seems to be unique to heimdal
  LSH_CHECK_KRB_LIB(krb5, krb5_verify_user_lrealm,, [enable_kerberos=no])
fi

AH_TEMPLATE([WITH_KERBEROS], [For kerberos])
if test x$enable_kerberos = xyes; then
  AC_DEFINE(WITH_KERBEROS)
  KRB_PROGRAM=lsh-krb-checkpw
fi

AC_SUBST(KRB_LIBS)
AC_SUBST(KRB_PROGRAM)

AH_TEMPLATE([WITH_PAM], [For PAM])
if test x$enable_pam = xyes; then
  AC_CHECK_LIB(pam,pam_start,, enable_pam=no )

  if test x$enable_pam = xyes; then
    AC_DEFINE(WITH_PAM)
    PAM_PROGRAM=lsh-pam-checkpw
  else
    AC_MSG_WARN([pam_start not found in libpam, disabling PAM])
  fi
fi

AC_SUBST(PAM_PROGRAM)

AH_TEMPLATE([WITH_GCOV], [Use gcov])
if test "x$enable_gcov" = "xyes"; then
  CFLAGS="$CFLAGS -ftest-coverage -fprofile-arcs"
  AC_DEFINE(WITH_GCOV)
fi

if test "x$enable_profiling" = "xyes"; then
  CFLAGS="$CFLAGS -pg"
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_UID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Needed by the supplied memcmp.c
AC_C_BIGENDIAN

# FIXME: We should check that a pid_t fits in an int
	
# Checks for library functions.

LSH_FUNC_ALLOCA
LSH_FUNC_STRERROR
LSH_FUNC_STRSIGNAL
AC_FUNC_MEMCMP
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(select socket strtol alarm)
AC_CHECK_FUNCS(getrusage gettimeofday)
AC_CHECK_FUNCS(getspnam)
AC_CHECK_FUNCS(vsnprintf inet_aton)
AC_CHECK_FUNCS(openpty)
AC_CHECK_FUNCS(cfmakeraw)
AC_CHECK_FUNCS(logwtmp login logout pututline pututxline updwtmp updwtmpx)
AC_CHECK_FUNCS(getaddrinfo getnameinfo gai_strerror)
AC_CHECK_FUNCS(setrlimit getdtablesize)
AC_FUNC_GETPGRP
AC_CHECK_FUNCS(syslog)

# HP-UX doesn't have seteuid
AC_CHECK_FUNCS(seteuid setresuid)

AH_BOTTOM(
[#if !HAVE_SETEUID
# if HAVE_SETRESUID
#  define seteuid(uid) setresuid(-1, (uid), -1)
# endif
#endif])

# FIXME: Is there a better way in autoconf 2.50?
AC_CACHE_CHECK([if netdb.h defines AI_NUMERICHOST],
  lsh_cv_sys_ai_numerichost,
  [AC_EGREP_CPP(yes,
    [#include <netdb.h>
     #ifdef AI_NUMERICHOST
     yes
     #endif
    ], lsh_cv_sys_ai_numerichost=yes,
       lsh_cv_sys_ai_numerichost=no)])

AH_TEMPLATE([HAVE_AI_NUMERICHOST], [Define if AI_NUMERICHOST exists])
if test x$lsh_cv_sys_ai_numerichost = xyes ; then
  AC_DEFINE(HAVE_AI_NUMERICHOST)
fi

AC_CHECK_FUNCS(poll,,[AC_LIBOBJ([jpoll])])

# Test if the libc includes a good enough argp.
if test x$with_system_argp = xyes ; then
  LSH_LIB_ARGP(,with_system_argp=no)
fi

# We don't use LIBOBJS for this, as the LIBOBJS are added to
# liblsh.a, and we can't add an archive to an archive.
LIBARGP=""
DOTDOT_LIBARGP=""
if test x$with_system_argp = xno ; then
  # FIXME: Perhaps it's better to use an absolute path?
  LIBARGP="argp/libargp.a"
  # Needed for linking in src/testsuite.
  DOTDOT_LIBARGP="../argp/libargp.a"
fi

AC_SUBST(LIBARGP)
AC_SUBST(DOTDOT_LIBARGP)

# Check for broken shutdown
AC_CACHE_CHECK(for working shutdown on AF_UNIX sockets,
	       lsh_cv_func_shutdown_works_on_unix_sockets,
[
AC_TRY_RUN([
#include <stdio.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <signal.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Creates a one-way socket connection. Returns 1 on success, 0 on
 * failure. fds[0] is for reading, fds[1] for writing (like for the
 * pipe() system call). */
static int make_pipe(int *fds)
{

#ifndef SHUT_RD
#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2
#endif

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
      fprintf(stderr, "socketpair() failed: %s\n", strerror(errno));
      return 0;
    }
  fprintf(stderr, "Created socket pair. Using fd:s %d <-- %d\n", fds[0], fds[1]);

  if(shutdown(fds[0], SHUT_WR) < 0)
    {
      fprintf(stderr, "shutdown(%d, SHUT_WR) failed: %s\n",
	      fds[0], strerror(errno));
      return 0;
    }
  if (shutdown(fds[1], SHUT_RD) < 0)
    {
      fprintf(stderr, "shutdown(%d, SHUT_RD) failed: %s\n",
	      fds[1], strerror(errno));
      return 0;
    }

  return 1;
}

int main(int argc, char **argv)
{
  int fds[2];
  
  if (!make_pipe(fds))
    exit(1);

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
      fprintf(stderr, "signal failed.\n");
      exit(1);
    }

  if ( (write(fds[1], "foo", 3) < 0)
       && (errno == EPIPE))
    exit(1);

  exit(0);
}
], lsh_cv_func_shutdown_works_on_unix_sockets=yes,
   lsh_cv_func_shutdown_works_on_unix_sockets=no,
   lsh_cv_func_shutdown_works_on_unix_sockets=yes)])

AH_TEMPLATE([SHUTDOWN_WORKS_WITH_UNIX_SOCKETS],
	    [Define to indicate that shutdown seems to work properly])
if test x$lsh_cv_func_shutdown_works_on_unix_sockets = xyes ; then
    AC_DEFINE(SHUTDOWN_WORKS_WITH_UNIX_SOCKETS)
fi

AC_CACHE_CHECK(
  [for working UNIX98 ptys],
  lsh_cv_sys_unix98_ptys,
  AC_TRY_RUN([
  /* Try to obtain a working UNIX98-style pty master and slave */
  /* Based on example code from the GNU C library documentation */
  #include <stdlib.h>	/* EXIT_FAILURE, EXIT_SUCCESS, ptsname */
  #include <unistd.h>	/* close(2) */
  
  #include <sys/types.h>	/* open(2) */
  #include <sys/stat.h>	/* - " -   */
  #ifdef HAVE_FCNTL_H
  #  include <fcntl.h>	/* - " -   */
  #endif
  #ifdef HAVE_STROPTS_H
  #  include <stropts.h>	/* isastream() */
  #endif
  
  int main(int argc, char* argv[]) {
    int master, slave;
    char *name;
  
    master = open("/dev/ptmx", O_RDWR); /* PTY master multiplex */
    if (master < 0) {
  	  exit(EXIT_FAILURE);
    }
  
    if (grantpt(master) < 0 || unlockpt(master) < 0)
      goto close_master;
    name = ptsname(master);
    if (name == NULL)
      goto close_master;
  
    slave = open(name, O_RDWR);
    if (slave == -1)
      goto close_master;
  
  #ifdef HAVE_STROPTS_H
    if (isastream(slave))
      {
  	if (ioctl(slave, I_PUSH, "ptem") < 0
  	    || ioctl(slave, I_PUSH, "ldterm") < 0)
  	  goto close_slave;
      }
  #endif
  
    exit(0);
  
  close_slave:
    close (slave);
  
  close_master:
    close (master);
    exit(1);
  }
  ],
  [lsh_cv_sys_unix98_ptys=yes],
  [lsh_cv_sys_unix98_ptys=no],
  # Pessimistic default for cross compilation.
  [lsh_cv_sys_unix98_ptys=no]))

AH_TEMPLATE([HAVE_UNIX98_PTYS],
	    [Define if a we have working UNIX98 pty handling])
if test x$lsh_cv_sys_unix98_ptys = xyes; then
  AC_DEFINE(HAVE_UNIX98_PTYS)
fi

LSH_GCC_ATTRIBUTES

AC_MSG_CHECKING(for BSD pty names)

AH_TEMPLATE([PTY_BSD_SCHEME_FIRST_CHARS],
	    [Possible first characters in a /dev/ptyXX name])
AH_TEMPLATE([PTY_BSD_SCHEME_SECOND_CHARS],
	    [Possible second characters in a /dev/ptyXX name])

# FIXME: What is most portable, tr -d '\n' or tr -d '\012' ?
AC_DEFINE_UNQUOTED(PTY_BSD_SCHEME_FIRST_CHARS, "`ls /dev/pty* | cut -c 9-9 | uniq | tr -d '\n'`")
AC_DEFINE_UNQUOTED(PTY_BSD_SCHEME_SECOND_CHARS, "`ls /dev/pty* | cut -c 10-10 | sort | uniq | tr -d '\n'`")

# FIXME: How can we test if the bsd scheme is likely to work?
AH_TEMPLATE([PTY_BSD_SCHEME], [Traditional BSD pty handling])
AC_DEFINE(PTY_BSD_SCHEME)
AC_MSG_RESULT([done])

# IPv6 support

if test x$enable_ipv6 = xyes ; then
  if test x$ac_cv_func_getaddrinfo != xyes ; then
    AC_MSG_WARN([getaddrinfo not found. Disabling IPv6 support])
    enable_ipv6=no
  elif test x$ac_cv_func_gai_strerror != xyes ; then
    AC_MSG_WARN([gai_strerror not found. Disabling IPv6 support])
    enable_ipv6=no
  elif test x$lsh_cv_sys_ai_numerichost != xyes ; then
    AC_MSG_WARN([AI_NUMERICHOST not defined. Disabling IPv6 support])
    enable_ipv6=no
  fi
fi

AH_TEMPLATE([WITH_IPV6], [For Ipv6 support])
if test x$enable_ipv6 = xyes ; then
  AC_DEFINE(WITH_IPV6)
fi

AC_CONFIG_SUBDIRS(src/argp)
AC_CONFIG_SUBDIRS(src/spki)
AC_CONFIG_SUBDIRS(src/sftp)

# Set these flags *last*, or else the test programs won't compile
if test x$GCC = xyes ; then
  # Using -ggdb3 makes (some versions of) Redhat's gcc-2.96 dump core
  if "$CC" --version | grep '^2\.96$' 1>/dev/null 2>&1; then
    true
  else
    CFLAGS="$CFLAGS -ggdb3"
  fi
  # FIXME: It would be better to actually test if this option works and/or is needed.
  if "$CC" --version | grep 'gcc.* 4\.' 1>/dev/null 2>&1; then
    CFLAGS="$CFLAGS -Wno-pointer-sign"
  fi
  CFLAGS="$CFLAGS -Wall -W \
  -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes \
  -Wpointer-arith -Wbad-function-cast -Wnested-externs"

# Don't enable -Wcast-align as it results in tons of warnings in the
# DES code. And when using stdio.
# Don't enable -Waggregate-return, as that causes warnings for glibc
# inttypes.h.

fi

AC_SUBST(BUILD_ARCH, `uname -p`)
AC_SUBST(BUILD_OSSYS,`uname -s`)
AC_SUBST(BUILD_OSREV,`uname -r`)

AC_CONFIG_FILES([Makefile doc/Makefile misc/Makefile
	  src/Makefile src/scm/Makefile
	  src/testsuite/Makefile src/rsync/Makefile
	  contrib/Makefile contrib/lsh.spec contrib/solpkg.sh])
AC_OUTPUT
