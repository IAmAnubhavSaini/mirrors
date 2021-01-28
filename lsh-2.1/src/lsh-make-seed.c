/* lsh-make-seed.c
 *
 * Creates an initial yarrow seed file
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2001 Niels Möller
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

/* The zlib using code isn't used now */
#undef WITH_ZLIB
#define WITH_ZLIB 0

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <termios.h>

/* getpwnam needed to lookup the user "nobody" */
#include <pwd.h>

/* It seems setgroups isn't defined in unistd.h */
#include <grp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#ifdef HAVE_POLL
# if HAVE_POLL_H
#  include <poll.h>
# elif HAVE_SYS_POLL_H
#  include <sys/poll.h>
# endif
#else
# include "jpoll.h"
#endif

/* Workaround for some version of FreeBSD. */
#ifdef POLLRDNORM
# define MY_POLLIN (POLLIN | POLLRDNORM)
#else /* !POLLRDNORM */
# define MY_POLLIN POLLIN
#endif /* !POLLRDNORM */


#if WITH_ZLIB
# if HAVE_ZLIB_H
#  include <zlib.h>
# endif
#endif /* WITH_ZLIB */

#include <nettle/yarrow.h>

#include "environ.h"
#include "format.h"
#include "io.h"
#include "lock_file.h"
#include "lsh_string.h"
#include "version.h"
#include "werror.h"
#include "xalloc.h"

#if WITH_ZLIB
/* FIXME: Duplicated in zlib.c */

/* zlib memory functions */
static void *
zlib_alloc(void *opaque UNUSED, unsigned int items, unsigned int size)
{
  return lsh_space_alloc(items * size);
}

static void
zlib_free(void *opaque UNUSED, void *address)
{
  lsh_space_free(address);
}
#endif /* WITH_ZLIB */

#include "lsh-make-seed.c.x"

/* Option parsing */

const char *argp_program_version
= "lsh-make-seed-" VERSION;

const char *argp_program_bug_address = BUG_ADDRESS;

#define OPT_SLOPPY 0x200
#define OPT_SERVER 0x201

/* GABA:
   (class
     (name lsh_make_seed_options)
     (vars
       ; Directory that should be created if needed
       (directory string)
       (filename string)
       (force . int)
       (sloppy . int)))
*/

static struct lsh_make_seed_options *
make_options(void)
{
  NEW(lsh_make_seed_options, self);

  self->directory = NULL;
  self->filename = NULL;
  self->force = 0;
  
  return self;
}

static const struct argp_option
main_options[] =
{
  /* Name, key, arg-name, flags, doc, group */
  { "output-file", 'o', "Filename", 0, "Default is ~/.lsh/seed-file", 0 },
  { "server", OPT_SERVER, NULL, 0,
    "Save seed file where the lshd server expects it", 0 },
  { "force", 'f', NULL, 0, "Overwrite any existing seed file.", 0 },
  { "sloppy", OPT_SLOPPY, NULL, 0, "Generate seed file even if we can't "
    "collect a good amount of randomness from the environment.", 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};
  
static const struct argp_child
main_argp_children[] =
{
  { &werror_argp, 0, "", 0 },
  { NULL, 0, NULL, 0}
};

static error_t
main_argp_parser(int key, char *arg, struct argp_state *state)
{
  CAST(lsh_make_seed_options, self, state->input);

  switch(key)
    {
    default:
      return ARGP_ERR_UNKNOWN;

    case ARGP_KEY_INIT:
      state->child_inputs[0] = NULL;
      break;

    case ARGP_KEY_END:
      if (!self->filename)
	{
	  char *home = getenv(ENV_HOME);
	  
	  if (!home)
	    {
	      argp_failure(state, EXIT_FAILURE, 0, "$HOME not set.");
	      return EINVAL;
	    }
	  else
	    {
	      self->directory = ssh_format("%lz/.lsh", home);
	      self->filename = ssh_format("%lz/.lsh/yarrow-seed-file", home);
	    }
	}
      break;
      
    case 'o':
      if (self->filename)
	argp_error(state, "You can use at most one -o or --server option.");
      else
	self->filename = make_string(arg);
      break;
      
    case OPT_SLOPPY:
      self->sloppy = 1;
      break;
      
    case OPT_SERVER:
      if (self->filename)
	argp_error(state, "You can use at most one -o or --server option.");
      else
	{
	  self->directory = make_string("/var/spool/lsh");
	  self->filename =  make_string("/var/spool/lsh/yarrow-seed-file");
	}
      break;
      
    case 'f':
      self->force = 1;
      break;
    }
  
  return 0;
}

static const struct argp
main_argp =
{ main_options, main_argp_parser, 
  NULL,
  "Creates an initial random seed file for the YARROW pseudorandomness"
  "generator used by lsh.",
  main_argp_children,
  NULL, NULL
};


/* For cleanup. FIXME: Arrange so that we clean up even if killed by
 * SIGINT or SIGTERM. */
static struct resource *lock = NULL;
int tty_needs_reset = 0;
struct termios tty_original_mode;

static void
cleanup(void)
{
  if (lock)
    KILL_RESOURCE(lock);

  if (tty_needs_reset)
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty_original_mode);
}

/* The sources we use. */
enum source_type
  {
    /* Data from /dev/random, if available */
    SOURCE_DEV_RANDOM,
    /* Data from /dev/mem, if we have permissions */
    SOURCE_DEV_MEM,
    /* Output from miscellaneous commands */
    SOURCE_SYSTEM,
    /* As a last resort, ask the user to type on the keyboard. */
    SOURCE_USER,

    /* Number of sources */
    NSOURCES
  };

#define DEVRANDOM_SIZE 40

/* FIXME: Add a similar function for egd, cryptlib tries to read
 * "/var/run/egd-pool", "/dev/egd-pool", "/etc/egd-pool". */

static void
get_dev_random(struct yarrow256_ctx *ctx, enum source_type source)
{
  static const char *names[] =
    { "/dev/random", "/dev/urandom",
      NULL };

  int fd = -1;
  unsigned i;
  int res;
  
  char buffer[DEVRANDOM_SIZE];

  for (i = 0; names[i]; i++)
    {
      fd = open(names[i], O_RDONLY);
      if (fd >= 0)
	break;
    }

  if (fd < 0)
    return;

  verbose("Reading %z...\n", names[i]);

  do
    { res = read(fd, buffer, DEVRANDOM_SIZE); }
  while ( (res < 0) && (errno == EINTR));
  
  if (res < 0)
    werror("Reading from %z failed %e\n",
	   names[i], errno);

  else if (res > 0)
    {
      /* Count 4 bits of entropy for each byte. */
      verbose("Read %i bytes from /dev/random.\n",
              res);
      yarrow256_update(ctx, source, res * 4, res, buffer);
    }
  else
    werror("unix_random.c: No data available on %z\n",
	   names[i]);
  
  close(fd);
}

#define BUF_SIZE 16384

#if WITH_ZLIB
static int
update_zlib(struct yarrow256_ctx *ctx, enum source_type source,
            z_stream *z, int flush,
            unsigned length, uint8_t *buf)
{
  /* Maximum expansion is 0.1% + 12 bytes. We use 1% + 12, to be
   * conservative.
   *
   * FIXME: These figures are documented for the entire stream,
   * does they really apply to all segments? */
  
  uint8_t out[BUF_SIZE + BUF_SIZE / 100 + 12];

  unsigned compressed;
  int rc;
  
  z->next_in = buf;
  z->avail_in = length;
  z->next_out = out;
  z->avail_out = sizeof(out);
  
  if ((rc = deflate(z, flush)) != Z_OK)
    {
      werror("deflate failed: %z\n",
             z->msg ? z->msg : "No error(?)");
      werror("rc = %i, flush = %i, avail_in = %i, avail_out = %i\n",
             rc, flush, z->avail_in, z->avail_out);
      return 0;
    }

  compressed = z->next_out - out;
          
  yarrow256_update(ctx, source, compressed / 1024, compressed, out);

  return 1;
}
#endif /* WITH_ZLIB */

static void
get_dev_mem(struct yarrow256_ctx *ctx, enum source_type source)
{
  /* /dev/mem provides access to physical memory, if we
   * have enough privileges.
   *
   * Count 1 bit of entropy compressed KB, or per 10 KB uncompressed.
   */

  /* FIXME: Reading /dev/mem in this was is inappropriate, as we may
   * accidentally read memory mapped i/o devices. How do we get oonly
   * the RAM? */
#if 0
  
  int fd = open("/dev/mem", O_RDONLY);
  
  if (fd < 0)
    {
      if (!getuid() || (errno != EACCES))
        werror("Couldn't open /dev/mem %e\n", errno);
    }
  else
    {
      char buf[BUF_SIZE];
      size_t total_read = 0;

#if WITH_ZLIB
      z_stream z;

      z.zalloc = zlib_alloc;
      z.zfree = zlib_free;
      
      if (deflateInit(&z, Z_DEFAULT_COMPRESSION) != Z_OK)
        {
          werror("deflateInit failed: %z\n",
                 z.msg ? z.msg : "No error(?)");
          close(fd);

          return;
        }
#endif /* WITH_ZLIB */

      werror("Reading /dev/mem...\n");
      for (;;)
        {
          int res;
          do
            res = read(fd, buf, BUF_SIZE);
          while ( (res < 0) && (errno != EINTR));

          if (!res)
            break;
          if (res < 0)
            {
              werror("Reading /dev/mem failed %e\n", errno);
              break;
            }

          total_read += res;
#if WITH_ZLIB
          if (!update_zlib(ctx, source,
                           &z, 0,
                           res, buf))
            break;
          
#else /* !WITH_ZLIB */
          yarrow256_update(ctx, source, res / 10240, res, buf);
#endif /* !WITH_ZLIB */
        }
#if WITH_ZLIB

      update_zlib(ctx, source,
                  &z, Z_FINISH,
                  0, buf);

      verbose("Read %i MB from /dev/mem, %i MB after compression.\n",
              total_read >> 20, z.total_out >> 20);
      
      deflateEnd(&z);

#else /* !WITH_ZLIB */
      verbose("Read %i MB from /dev/mem.\n",
              total_read >> 20);
      
#endif /* !WITH_ZLIB */

      close(fd);
    }
#endif
}

/* List of commands based on Peter Gutmann's cryptlib,
 * misc/rndunix.c. <URL:
 * http://www.cs.auckland.ac.nz/~pgut001/cryptlib/> */

#if 0
static struct RI {
	const char *path;		/* Path to check for existence of source */
	const char *arg;		/* Args for source */
	const int usefulness;	/* Usefulness of source */
	FILE *pipe;				/* Pipe to source as FILE * */
	int pipeFD;				/* Pipe to source as FD */
	pid_t pid;				/* pid of child for waitpid() */
	int length;				/* Quantity of output produced */
	const BOOLEAN hasAlternative;	/* Whether source has alt.location */
	} dataSources[] = {
	{ "/bin/vmstat", "-s", SC( -3 ), NULL, 0, 0, 0, TRUE },
	{ "/usr/bin/vmstat", "-s", SC( -3 ), NULL, 0, 0, 0, FALSE },
	... };
#endif

/* FIXME: Are we too conservative here? Most sources get credited with
 * only one or two bits per KB of output. On my system, get_system
 * estimates a total of eight bits of entropy... */
#define UNIX_RANDOM_SOURCE_SCALE 8192

struct unix_random_source
{
  const char *path;
  const char *arg; /* For now, use at most one argument. */
  int has_alternative;

  /* If non-zero, count quality bits of entropy (corresponding to 1K)
   * if the amount of output exceeds this value. */
  unsigned small;
  /* Bits of entropy per UNIX_RANDOM_SOURCE_SCALE bytes of output. */
  unsigned quality;
};

/* Lots of output expected; round downwards. */
#define WLARGE(x) 0, x
/* Small but significant output expected; round upwards */
#define WSMALL(x) 100, x

static const struct unix_random_source
system_sources[] = {
  { "/bin/vmstat", "-s", 1, WSMALL(30) },
  { "/usr/bin/vmstat", "-s", 0, WSMALL(30) },
  { "/bin/vmstat", "-c", 1, WSMALL(30) },
  { "/usr/bin/vmstat", "-c", 0, WSMALL(30) },
  { "/usr/bin/pfstat", NULL, 0, WSMALL(20) },
  { "/bin/vmstat", "-i", 1, WSMALL(20) },
  { "/usr/bin/vmstat", "-i", 0, WSMALL(20) },
  { "/usr/ucb/netstat", "-s", 1, WLARGE(20) },
  { "/usr/bin/netstat", "-s", 1, WLARGE(20) },
  { "/usr/sbin/netstat", "-s", 1, WLARGE(20) },
  { "/bin/netstat", "-s", 1, WLARGE(20) },
  { "/usr/etc/netstat", "-s", 0, WLARGE(20) },
  { "/usr/bin/nfsstat", NULL, 0, WLARGE(20) },
  { "/usr/ucb/netstat", "-m", 1, WSMALL(10) },
  { "/usr/bin/netstat", "-m", 1, WSMALL(10) },
  { "/usr/sbin/netstat", "-m", 1, WSMALL(10) },
  { "/bin/netstat", "-m", 1, WSMALL(10) },
  { "/usr/etc/netstat", "-m", 0, WSMALL(10) },
  { "/usr/ucb/netstat", "-in", 1, WSMALL(10) },
  { "/usr/bin/netstat", "-in", 1, WSMALL(10) },
  { "/usr/sbin/netstat", "-in", 1, WSMALL(10) },
  { "/bin/netstat", "-in", 1, WSMALL(10) },
  { "/usr/etc/netstat", "-in", 0, WSMALL(10) },
  { "/usr/sbin/ntptrace", "-r2 -t1 -nv", 0, WSMALL(10) },
  { "/usr/sbin/snmp_request", "localhost public get 1.3.6.1.2.1.7.1.0",

    0, WSMALL(10) }, /* UDP in */
  { "/usr/sbin/snmp_request", "localhost public get 1.3.6.1.2.1.7.4.0",
    0, WSMALL(10) }, /* UDP out */
  { "/usr/sbin/snmp_request", "localhost public get 1.3.6.1.2.1.4.3.0",
    0, WSMALL(10) }, /* IP ? */
  { "/usr/sbin/snmp_request", "localhost public get 1.3.6.1.2.1.6.10.0",
    0, WSMALL(10) }, /* TCP ? */
  { "/usr/sbin/snmp_request", "localhost public get 1.3.6.1.2.1.6.11.0",
    0, WSMALL(10) }, /* TCP ? */
  { "/usr/sbin/snmp_request", "localhost public get 1.3.6.1.2.1.6.13.0",
    0, WSMALL(10) }, /* TCP ? */
  { "/usr/bin/mpstat", NULL, 0, WLARGE(10) },
  { "/usr/bin/w", NULL, 1, WLARGE(10) },
  { "/usr/bsd/w", NULL, 0, WLARGE(10) },
  { "/usr/bin/df", NULL, 1, WLARGE(10) },
  { "/bin/df", NULL, 0, WLARGE(10) },
  { "/usr/sbin/portstat", NULL, 0, WLARGE(10) },
  { "/usr/bin/iostat", NULL, 0, WLARGE(0) },
  { "/usr/bin/uptime", NULL, 1, WLARGE(0) },
  { "/usr/bsd/uptime", NULL, 0, WLARGE(0) },
  { "/bin/vmstat", "-f", 1, WLARGE(0) },
  { "/usr/bin/vmstat", "-f", 0, WLARGE(0) },
  { "/bin/vmstat", NULL, 1, WLARGE(0) },
  { "/usr/bin/vmstat", NULL, 0, WLARGE(0) },
  { "/usr/ucb/netstat", "-n", 1, WLARGE(5) },
  { "/usr/bin/netstat", "-n", 1, WLARGE(5) },
  { "/usr/sbin/netstat", "-n", 1, WLARGE(5)  },
  { "/bin/netstat", "-n", 1, WLARGE(5)  },
  { "/usr/etc/netstat", "-n", 0, WLARGE(5)  },
#if defined( __sgi ) || defined( __hpux )
  { "/bin/ps", "-el", 1, WLARGE(3) },
#endif /* __sgi || __hpux */
  { "/usr/ucb/ps", "aux", 1, WLARGE(3) },
  { "/usr/bin/ps", "aux", 1, WLARGE(3) },
  { "/bin/ps", "aux", 0, WLARGE(3) },
  { "/usr/bin/ipcs", "-a", 1, WLARGE(5) },
  { "/bin/ipcs", "-a", 0, WLARGE(5) },
  /* Unreliable source, depends on system usage */
  { "/etc/pstat", "-p", 1, WLARGE(5) },
  { "/bin/pstat", "-p", 0, WLARGE(5) },
  { "/etc/pstat", "-S", 1, WLARGE(2) },
  { "/bin/pstat", "-S", 0, WLARGE(2) },
  { "/etc/pstat", "-v", 1, WLARGE(2) },
  { "/bin/pstat", "-v", 0, WLARGE(2) },
  { "/etc/pstat", "-x", 1, WLARGE(2) },
  { "/bin/pstat", "-x", 0, WLARGE(2) },
  { "/etc/pstat", "-t", 1, WLARGE(1) },
  { "/bin/pstat", "-t", 0, WLARGE(1) },
  /* pstat is your friend */
  { "/usr/bin/last", "-n 50", 1, WLARGE(3) },
#ifdef __sgi
  { "/usr/bsd/last", "-50", 0, WLARGE(3) },
#endif /* __sgi */
#ifdef __hpux
  { "/etc/last", "-50", 0, WLARGE(3) },
#endif /* __hpux */
  { "/usr/bsd/last", "-n 50", 0, WLARGE(3) },
#ifdef sun
  { "/usr/bin/showrev", "-a", 0, WLARGE(1) },
  { "/usr/sbin/swap", "-l", 0, WLARGE(0) },
  { "/usr/sbin/prtconf", "-v", 0, WLARGE(0) },
#endif /* sun */
  { "/usr/sbin/psrinfo", NULL, 0, WLARGE(0) },
  { "/usr/local/bin/lsof", "-lnwP", 0, WLARGE(3) },
  /* Output is very system and version-dependent */
  { "/usr/sbin/snmp_request", "localhost public get 1.3.6.1.2.1.5.1.0",
    0, WLARGE(1) }, /* ICMP ? */
  { "/usr/sbin/snmp_request", "localhost public get 1.3.6.1.2.1.5.3.0",
    0, WLARGE(1) }, /* ICMP ? */
  { "/etc/arp", "-a", 1, WLARGE(1) },
  { "/usr/etc/arp", "-a", 1, WLARGE(1) },
  { "/usr/bin/arp", "-a", 1, WLARGE(1) },
  { "/usr/sbin/arp", "-a", 0, WLARGE(1) },
  { "/usr/sbin/ripquery", "-nw 1 127.0.0.1", 0, WLARGE(1) },
  { "/bin/lpstat", "-t", 1, WLARGE(1) },
  { "/usr/bin/lpstat", "-t", 1, WLARGE(1) },
  { "/usr/ucb/lpstat", "-t", 0, WLARGE(1) },
  { "/usr/bin/tcpdump", "-c 5 -efvvx", 0, WLARGE(10) },
  /* This is very environment-dependant.  If
     network traffic is low, it'll probably time
     out before delivering 5 packets, which is OK
     because it'll probably be fixed stuff like
     ARP anyway */
  { "/usr/sbin/advfsstat", "-b usr_domain", 0, WLARGE(0) },
  { "/usr/sbin/advfsstat", "-l 2 usr_domain", 0, WLARGE(5) },
  { "/usr/sbin/advfsstat", "-p usr_domain", 0, WLARGE(0) },

  /* This is a complex and screwball program.  Some
     systems have things like rX_dmn, x = integer,
     for RAID systems, but the statistics are
     pretty dodgy */
#if 0
  /* The following aren't enabled since they're somewhat slow and not very
     unpredictable, however they give an indication of the sort of sources
     you can use (for example the finger might be more useful on a
     firewalled internal network) */
  { "/usr/bin/finger", "@ml.media.mit.edu", 0, WLARGE(9) },
  { "/usr/local/bin/wget", "-O - http://lavarand.sgi.com/block.html",
    0, WLARGE(9) },
  { "/bin/cat", "/usr/spool/mqueue/syslog", 0, WLARGE(9) },
#endif /* 0 */
  { NULL, NULL, 0, 0, 0 }
};

struct unix_random_source_state
{
  const struct unix_random_source *source;
  pid_t pid;       /* Running process. */
  int fd;
  unsigned length; /* Amount of data read so far. */

  unsigned entropy; /* Entropy estimate. Used for status messages
		     * only, its yarrow's internal counters that matter
		     * for reseeding. */

  unsigned remainder;   /* Partial entropy, number of bits times
			 *  UNIX_RANDOM_SOURCE_SCALE */
};

static int
spawn_source_process(unsigned *index,
		     struct unix_random_source_state *state,
		     int dev_null, uid_t uid, gid_t gid)
{
  unsigned i;
  pid_t pid;
  /* output[0] for reading, output[1] for writing. */
  int output[2];
      
  for (i = *index; system_sources[i].path; )
    {
      if (access(system_sources[i].path, X_OK) < 0)
	{
	  debug("unix_random.c: spawn_source_process: Skipping '%z'; not executable %e\n",
		system_sources[i].path, errno);
	  i++;
	}
      else
	break;
    }

  if (!system_sources[i].path)
    {
      *index = i;
      return 0;
    }
  
  if (!lsh_make_pipe(output))
    {
      werror("spawn_source_process: Can't create pipe %e\n", errno);
      return 0;
    }
  
  state->source = system_sources + i;
  
  if (!system_sources[i].has_alternative)
    i++;
  else
    /* Skip alternatives */
    while (system_sources[i].has_alternative)
      i++;
  
  *index = i;

  verbose("Starting %z %z\n",
	  state->source->path,
	  state->source->arg ? state->source->arg : "");
  
  pid = fork();
  switch(pid)
    {
    default:
      /* Parent */
      close (output[1]);
      state->fd = output[0];
      state->pid = pid;
      io_set_close_on_exec(state->fd);
      io_set_nonblocking(state->fd);

      state->length = 0;
      state->entropy = 0;
      state->remainder = 0;
      
      return 1;
      
    case -1:
      /* Error */
      close(output[0]);
      close(output[1]);
      return 0;
    case 0:
      /* Child */
      /* Change uid to nobody */
      if (uid)
	{
	  if (setgroups(0, NULL) < 0)
	    {
	      werror("Failed to clear supplimentary groups list %e\n", errno);
	      _exit(EXIT_FAILURE);
	    }
	      
	  if (setgid(gid) < 0)
	    {
	      werror("Failed to change gid %e\n", errno);
	      _exit(EXIT_FAILURE);
	    }
	  if (setuid(uid) < 0)
	    {
	      werror("Failed to change uid %e\n", errno);
	      _exit(EXIT_FAILURE);
	    }
	}
	  
      if (dup2(output[1], STDOUT_FILENO) < 0)
	{
	  werror("spawn_source_process: dup2 for stdout failed %e\n", errno);
	  _exit(EXIT_FAILURE);
	}

      /* Ignore stderr. */
      if (dup2(dev_null, STDERR_FILENO) < 0)
	{
	  werror("spawn_source_process: dup2 for stderr failed %e\n", errno);
	  _exit(EXIT_FAILURE);
	}
	
      close (output[0]);
      close (output[1]);
      close (dev_null);

      /* Works also if state->source->arg == NULL */
      execl(state->source->path, state->source->path,
	    state->source->arg, NULL);
      
      werror("spawn_source_process: execl '%z' failed %e\n",
	     state->source->path, errno);
      
      _exit(EXIT_FAILURE);
    }
}

/* Figure out the uid and gid we should run the randomness polling
 * sub-processes as. Set to zero if we shouldn't try changing our
 * identity. */
static void
nobody_ids(uid_t *uid, gid_t *gid)
{
  if (getuid())
    /* We're not running as root, so don't try changing uid. */
    *uid = *gid = 0;
  else
    {
      struct passwd *pw = getpwnam("nobody");
      if (pw)
	{
	  *uid = pw->pw_uid;
	  *gid = pw->pw_gid;
	}
      else
	{
	  werror("No user `nobody' found. will run processes as -1:-1\n");
	  *uid = (uid_t) -1;
	  *gid = (gid_t) -1;
	}
    }
}

/* Spawn this number of processes. */
#define UNIX_RANDOM_POLL_PROCESSES 10

/* Count entropy bits of entropy if we can read at least limit bytes
 * of data. */
struct unix_proc_source
{
  const char *name;
  unsigned limit;
  unsigned entropy;
};

static const struct unix_proc_source
linux_proc_sources[] = {
  /* Say we have four kinds of interrupts, three of which provide 2
   * bits of entropy each. */
  { "/proc/interrupts", 100, 6 },
  /* Five values, count about half a bit for each. */
  { "/proc/loadavg", 10, 3 },
  /* Fairly static information, about 150 bytes per lock.
   * Count 10 bits if we have more than 10 locks. */
  { "/proc/locks", 1500 , 10 },
  /* Count 1 bits each for 5 of the values. */
  { "/proc/meminfo", 300, 5 },
  /* 12 lines, count 1 bits per line. */
  { "/proc/stat", 300, 12 },
  /* About 30 lines, count 1 bit each for five of those. */
  { "/proc/slabinfo", 800, 5 },
  /* Count 5 bits if have more ten 20 lines. Use the same estimate
   * for all these network types. */
  { "/proc/net/tcp", 2000, 10 },
  { "/proc/net/udp", 2000, 10 },
  { "/proc/net/ipx", 2000, 10 },

  /* We should have at least two interfaces. Coutn 5 bits for the
   * information for the primary interface. */
  { "/proc/net/dev", 400, 5 },
  { NULL, 0, 0 }
};
  
static void
get_system(struct yarrow256_ctx *ctx, enum source_type source)
{
  uid_t uid;
  gid_t gid;

  struct unix_random_source_state state[UNIX_RANDOM_POLL_PROCESSES];
  unsigned running = 0;
  unsigned i;
  unsigned index = 0;
  
  int dev_null;

  unsigned count = 0;

  struct {
    pid_t pid;
    struct timeval now;
  } unique;

  werror("Reading system state...\n");

  nobody_ids(&uid, &gid);

  dev_null = open("/dev/null", O_WRONLY);

  if (dev_null < 0)
    {
      werror("Failed to open /dev/null %e\n", errno);
      return;
    }

  /* Make sure two runs don't get exactly the same data */
  unique.pid = getpid();
  if (gettimeofday(&unique.now, NULL) < 0)
    {
      werror("getimeofday failed %e\n", errno);
      return;
    }

  yarrow256_update(ctx, source,
		   0,
		   sizeof(unique), (uint8_t *) &unique);
  
  for (i = 0; i < UNIX_RANDOM_POLL_PROCESSES; i++)
    state[i].fd = -1;

  for (running = 0; running<UNIX_RANDOM_POLL_PROCESSES; running++)
    {
      if (!spawn_source_process(&index, state + running, dev_null,
				uid, gid))
	break;
    }

  while (running)
    {
      struct pollfd fds[UNIX_RANDOM_POLL_PROCESSES];
      unsigned nfds;
      unsigned i;

      for (i = 0, nfds = 0; i < UNIX_RANDOM_POLL_PROCESSES; i++)
	if (state[i].fd > 0)
	  {
	    fds[nfds].fd = state[i].fd;
	    fds[nfds].events = MY_POLLIN;
	    nfds++;
	  }

      assert(nfds == running);

      trace("get_system: calling poll, nfds = %i\n", nfds);
      if (poll(fds, nfds, -1) < 0)
	{
	  werror("get_system: poll failed %e\n", errno);
	  break;
	}

      trace("get_system: returned from poll.\n");

      for (i = 0; i<nfds; i++)
	{
	  trace("get_system: poll for fd %i: events = 0x%xi, revents = 0x%xi.\n",
		fds[i].fd, fds[i].events, fds[i].revents);

	  /* Linux sets POLLHUP on EOF */
#ifndef POLLHUP
#define POLLHUP 0
#endif
	  if (fds[i].revents & (MY_POLLIN | POLLHUP))
	    {
#define BUFSIZE 1024
	      uint8_t buffer[BUFSIZE];
	      int res;
	      unsigned j;
	    
	      for (j = 0; j<UNIX_RANDOM_POLL_PROCESSES; j++)
		if (fds[i].fd == state[j].fd)
		  break;

	      assert(j < UNIX_RANDOM_POLL_PROCESSES);

	      trace("get_system: reading from source %z\n",
		    state[j].source->path);
	      
	      res = read(fds[i].fd, buffer, BUFSIZE);
#undef BUFSIZE
	    
	      if (res < 0)
		{
		  if (errno != EINTR)
		    {
		      werror("get_system: background_poll read failed %e\n",
			     errno);
		      return;
		    }
		}
	      else if (res > 0)
		{
		  /* Estimate entropy */

		  unsigned entropy;
		  unsigned old_length = state[j].length;
		  state[j].length += res;
		  
		  state[j].remainder += state[j].source->quality * res;

		  /* Small sources are credited 1K of input as soon is
		   * we get the "small" number of input butes. */
		  if ( (old_length < state[j].source->small) &&
		       (state[j].length >= state[j].source->small) )
		    state[j].remainder += state[j].source->quality * 1024;

		  entropy = state[j].remainder / UNIX_RANDOM_SOURCE_SCALE;
		  if (entropy)
		    state[j].remainder %= UNIX_RANDOM_SOURCE_SCALE;

		  state[j].entropy += entropy;
		  
		  yarrow256_update(ctx, source,
				   entropy,
				   res, buffer);
		}
	      else
		{ /* EOF */
		  int status;
		
		  close(fds[i].fd);

		  state[j].fd = -1;

		  verbose("Read %i bytes from %z %z, entropy estimate: %i bits\n",
			  state[j].length,
			  state[j].source->path,
			  state[j].source->arg ? state[j].source->arg : "",
			  state[j].entropy);
		  count += state[j].entropy;
		  
		  if (waitpid(state[j].pid, &status, 0) < 0)
		    {
		      werror("waitpid failed %e\n", errno);
		      return;
		    }
		  if (WIFEXITED(status))
		    {
		      if (WEXITSTATUS(status))
			verbose("Command %z %z failed.\n",
				state[j].source->path,
				state[j].source->arg ? state[j].source->arg : "");
		    }
		  else if (WIFSIGNALED(status))
		    {
		      werror("Command %z %z died from signal %i (%z).\n",
			     state[j].source->path,
			     state[j].source->arg ? state[j].source->arg : "",
			     WTERMSIG(status),
			     STRSIGNAL(WTERMSIG(status)));
		    }

		  if (!spawn_source_process(&index, state + j, dev_null,
					    uid, gid))
		    running--;
		}
	    }
	}
    }
  
  for (i = 0; linux_proc_sources[i].name; i++)
    {
      int fd = open(linux_proc_sources[i].name, O_RDONLY);
      if (fd > 0)
	{
#define BUFSIZE 5000
	  uint8_t buffer[BUFSIZE];
#undef BUFSIZE
	  int res;
	  
	  do
	    res = read(fd, buffer, sizeof(buffer));
	  while ( (res < 0) && (errno == EINTR) );

	  if (res < 0)
	    werror("Reading %z failed %e\n",
		   linux_proc_sources[i].name, errno);
	  else
	    {
	      unsigned entropy = 0;
	      if (res > linux_proc_sources[i].limit)
		entropy = linux_proc_sources[i].entropy;

	      verbose("Read %i bytes from %z, entropy estimate: %i bits\n",
		      res, linux_proc_sources[i].name, entropy);
	      
	      yarrow256_update(ctx, source, entropy, res, buffer);

	      count += entropy;
	    }
	  close(fd);
	}
    }
  werror("Got %i bits of entropy from system state.\n", count);
}

static long
time_usec_diff(const struct timeval *first,
	       const struct timeval *second)
{
  return (second->tv_sec - first->tv_sec) * 1000000
    + (second->tv_usec - first->tv_usec);
}

static unsigned
get_time_accuracy(void)
{
  struct timeval start;
  struct timeval next;
  long diff;
  
  if (gettimeofday(&start, NULL))
    {
      werror("gettimeofday failed %e\n", errno);
      return 0;
    }

  do
    if (gettimeofday(&next, NULL))
      {
	werror("gettimeofday failed %e\n", errno);
	return 0;
      }
  
  while ( (next.tv_sec == start.tv_sec)
	  && (next.tv_usec == start.tv_usec));
  
  diff = time_usec_diff(&start, &next);

  if (diff <= 0)
    return 0;

  /* If accuract is worse than 0.1 second, it's no use. */
  if (diff > 100000)
    {
      werror("gettimeofday accuracy seems too bad to be useful");
      return 0;
    }
  
  return diff;
}


static void
get_interact(struct yarrow256_ctx *ctx, enum source_type source)
{
  unsigned accuracy = get_time_accuracy();
  unsigned count;
  unsigned progress;
  unsigned keys;
  
  struct yarrow_key_event_ctx estimator;

  if (!accuracy)
    return;

  verbose("gettimeofday accuracy: %i microseconds.\n",
	  accuracy);

  werror("Please type some random data. You better do this\n");
  werror("when connected directly to a console, typing over\n");
  werror("the network provides worse timing information, and\n");
  werror("more opportunities for eavesdropping.\n");

  werror_progress("----------------------------------------\n");
  
  yarrow_key_event_init(&estimator);

  if (!tcgetattr(STDIN_FILENO, &tty_original_mode))
    {
      struct termios tty_mode = tty_original_mode;

      tty_needs_reset = 1;
      
      tty_mode.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
      tty_mode.c_cflag &= ~(CSIZE|PARENB); tty_mode.c_cflag |= CS8;
      tty_mode.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
      tty_mode.c_cc[VMIN] = 1;
      tty_mode.c_cc[VTIME] = 0;
      
      tcsetattr(STDIN_FILENO, TCSADRAIN, &tty_mode);
    }
      
  for (count = 0, progress = 5, keys = 0; count < 200; )
    {
      struct {
	struct timeval now;
	int c;
      } event;

      int entropy;
      unsigned time;

      keys++;
      
      event.c = getchar();
      if (gettimeofday(&event.now, NULL) < 0)
	{
	  werror("gettimeofday failed %e\n", errno);
	  return;
	}

      if (event.c < 0)
	{
	  werror_progress("\n");
	  werror("Reading keystrokes failed.\n");
	  return;
	}

      /* Compute time as the number of seconds and microseconds
       * divided by accuracy. Taking the numer of seconds mod 1000
       * ensures that the calculation doesn't overflow. */

      time = ( (event.now.tv_sec % 1000) * 1000000 + event.now.tv_usec)
	/ accuracy;
      
      /* We only look at the microsecond data,  */
      entropy = yarrow_key_event_estimate(&estimator,
					  event.c, time);
      
      debug("Got char `%c', time: %i, entropy: %i\n", event.c, time, entropy);
      
      yarrow256_update(ctx, source,
		       entropy,
		       sizeof(event), (uint8_t *) &event);
      
      count += entropy;
      
      if (count >= progress)
	{
	  werror_progress(".");
	  progress += 5;
	}
    }
  
  werror_progress("\n");
  werror("Got %i keystrokes, estimating %i bits of entropy.\n",
	 keys, count);
  
  werror("You can stop typing now.\n");
  
  if (tty_needs_reset)
    {
      /* Wait a moment for the user to stop typing */
      sleep(1);
      
      /* Reset terminal mode, and disgard buffered input. */  
      tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty_original_mode);
      tty_needs_reset = 0;
    }
}

int
main(int argc, char **argv)
{
  struct lsh_make_seed_options *options = make_options();
  struct lsh_file_lock_info *lock_info;

  int overwrite = 0;
  
  const struct exception *e;
  int fd;

  struct yarrow256_ctx yarrow;
  struct yarrow_source sources[NSOURCES];
  uint8_t seed[YARROW256_SEED_FILE_SIZE];

  argp_parse(&main_argp, argc, argv, 0, NULL, options);

  if (atexit(cleanup) < 0)
    {
      werror("atexit failed!?\n");
      return EXIT_FAILURE;
    }

  if (options->directory
      && (mkdir(lsh_get_cstring(options->directory), 0755) < 0)
      && (errno != EEXIST) )
    {
      werror("Creating `%S' failed %e.\n",
	     options->directory, errno);
      return EXIT_FAILURE;
    }

  lock_info = make_lsh_file_lock_info(ssh_format("%lS.lock",
						 options->filename));

  /* Try to fail early. */
  if (LSH_FILE_LOCK_P(lock_info))
    {
      werror("File `%S' is locked\n", options->filename);
      return EXIT_FAILURE;
    }

  if (!options->force)
    {
      struct stat sbuf;
      if (stat(lsh_get_cstring(options->filename), &sbuf) == 0)
	{
	  werror("File `%S' already exists.\n", options->filename);
	  return EXIT_FAILURE;
	}
    }
    
  yarrow256_init(&yarrow, NSOURCES, sources);

  get_dev_random(&yarrow, SOURCE_DEV_RANDOM);
  get_dev_mem(&yarrow, SOURCE_DEV_MEM);
  get_system(&yarrow, SOURCE_SYSTEM);

  if (!yarrow256_is_seeded(&yarrow))
    {
      /* Get the number of additional sources that need to get above
       * the reseed threshold before a reseed happens. */
      if (!options->sloppy && (yarrow256_needed_sources(&yarrow) > 1))
        {
          werror("Couldn't get enough randomness from the environment.\n");

          return EXIT_FAILURE;
        }
      if (!quiet_flag)
	get_interact(&yarrow, SOURCE_USER);
    }

  if (!options->sloppy && !yarrow256_is_seeded(&yarrow))
    {
      werror("Couldn't get enough randomness from the environment.\n");

      return EXIT_FAILURE;
    }

  yarrow256_force_reseed(&yarrow);

  lock = LSH_FILE_LOCK(lock_info, 5);

  if (!lock)
    {
      werror("Failed to lock file `%S'\n", options->filename);
      return EXIT_FAILURE;
    }

  /* Create file, readable only be the user. */
  fd = open(lsh_get_cstring(options->filename),
	    O_EXCL | O_CREAT | O_WRONLY,
	    0600);

  if (options->force && (fd < 0) && (errno == EEXIST))
    {
      werror("Overwriting `%S'\n",
	     options->filename);

      overwrite = 1;

      /* FIXME: Use O_TRUNC? */
      fd = open(lsh_get_cstring(options->filename),
		O_WRONLY,
		0600);
    }
  
  if (fd < 0)
    {
      werror("Failed to open file `%S' %e\n",
	     options->filename, errno);

      KILL_RESOURCE(lock);
      return EXIT_FAILURE;
    }

  if (overwrite)
    {
      /* If we're overwriting an existing file, make sure it has
       * reasonable permissions */

      struct stat sbuf;
      if (fstat(fd, &sbuf) < 0)
	{
	  werror("Failed to stat file `%S' %e\n",
		 options->filename, errno);

	  close(fd);
	  KILL_RESOURCE(lock);
	  return EXIT_FAILURE;
	}

      if (sbuf.st_uid != getuid())
	{
	  werror("The file `%S' is owned by somebody else.\n");

	  close(fd);
	  KILL_RESOURCE(lock);
	  return EXIT_FAILURE;
	}

      if (sbuf.st_mode & (S_IRWXG | S_IRWXO))
	{
	  werror("Too permissive permissions on `%S', trying to fix it.\n",
		 options->filename);
	  if (fchmod(fd, sbuf.st_mode & ~(S_IRWXG | S_IRWXO)) < 0)
	    {
	      werror("Failed to change permissions %e\n", errno);
	      close(fd);
	      KILL_RESOURCE(lock);
	      return EXIT_FAILURE;
	    }
	}

      /* FIXME: Use O_TRUNC instead? */
      if (ftruncate(fd, 0) < 0)
	{
	  werror("Failed to truncate file `%S' %e\n",
		 options->filename, errno);

	  close(fd);
	  KILL_RESOURCE(lock);
	  return EXIT_FAILURE;
	}
    }
  
  yarrow256_random(&yarrow, sizeof(seed), seed);
  e = write_raw(fd, sizeof(seed), seed);

  if (e)
    {
      werror("Writing seed file failed: %z\n",
	     e->msg);

      /* If we're overwriting the file, it's already truncated now,
       * we can't leave it unmodified. So just delete it. */

      unlink(lsh_get_cstring(options->filename));
      close(fd);
      KILL_RESOURCE(lock);
      return EXIT_FAILURE;
    }
  
  KILL_RESOURCE(lock);
  close(fd);
  
  return EXIT_SUCCESS;
}
