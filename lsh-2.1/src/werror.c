/* werror.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998 Niels Möller
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

#include <assert.h>
#include <ctype.h>
#include <string.h>

#include <fcntl.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#if HAVE_SYSLOG_H
#include <syslog.h>
#endif

#include "werror.h"

#include "charset.h"

/* For format_size_in_decimal */
#include "format.h"

#include "gc.h"
#include "io.h"
#include "lsh_string.h"
#include "parse.h"
#include "xalloc.h"


/* Global flags */
int trace_flag = 0;
int debug_flag = 0;
int quiet_flag = 0;
int verbose_flag = 0;

static const char *program_name = NULL;

#define WERROR_TRACE -1
#define WERROR_DEBUG -2
#define WERROR_LOG -3

static const struct argp_option
werror_options[] =
{
  { "quiet", 'q', NULL, 0, "Suppress all warnings and diagnostic messages", 0 },
  { "verbose", 'v', NULL, 0, "Verbose diagnostic messages", 0},
  { "trace", WERROR_TRACE, NULL, 0, "Detailed trace", 0 },
  { "debug", WERROR_DEBUG, NULL, 0, "Print huge amounts of debug information", 0 },
  { "log-file", WERROR_LOG, "File name", 0,
    "Append messages to this file.", 0},
  { NULL, 0, NULL, 0, NULL, 0 }
};

static error_t
werror_argp_parser(int key, char *arg,
		   struct argp_state *state)
{
  switch(key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    case ARGP_KEY_END:
    case ARGP_KEY_INIT:
      program_name = state->name;
      break;
    case 'q':
      quiet_flag = 1;
      break;
    case 'v':
      verbose_flag = 1;
      break;
    case WERROR_TRACE:
      trace_flag = 1;
      break;
    case WERROR_DEBUG:
      debug_flag = 1;
      break;
    case WERROR_LOG:
      {
	/* FIXME: For clients, this is right: We only get lsh-related
	 * messages to the log file, and child processes are not
	 * affected. But for the server, perhaps we should also dup
	 * the logfile over stderr? */
	
	int fd = open(arg, O_WRONLY | O_CREAT | O_APPEND, 0666);
	if (fd < 0)
	  argp_error(state, "Failed to open log file `%s'.", arg);
	else
	  {
	    io_set_close_on_exec(fd);
	    set_error_stream(fd);
	  }
      }
    }
  return 0;
}

const struct argp werror_argp =
{
  werror_options,
  werror_argp_parser,
  NULL, NULL, NULL, NULL, NULL
};

static int error_fd = STDERR_FILENO;

#define BUF_SIZE 500
static uint8_t error_buffer[BUF_SIZE];
static uint32_t error_pos = 0;
static int error_raw = 0;

static const struct exception *
(*error_write)(int fd, uint32_t length, const uint8_t *data) = write_raw;

#if HAVE_SYSLOG
static const struct exception *
write_syslog(int fd UNUSED, uint32_t length, const uint8_t *data)
{
  struct lsh_string *s;

  /* Data must not contain any NUL:s */
  assert(!memchr(data, '\0', length));
  
  /* NUL-terminate the string. */
  s = ssh_format("%ls", length, data);

  /* FIXME: Should we use different log levels for werror, verbose and
   * debug? */
  
  syslog(LOG_NOTICE, "%s", lsh_get_cstring(s));
  lsh_string_free(s);
  
  return NULL;
}

/* FIXME: Delete argument and use program_name. */
void
set_error_syslog(const char *id)
{
  openlog(id, LOG_PID | LOG_CONS, LOG_DAEMON);
  error_write = write_syslog;
  error_fd = -1;
}
#endif /* HAVE_SYSLOG */

static const struct exception *
write_ignore(int fd UNUSED,
	     uint32_t length UNUSED, const uint8_t *data UNUSED)
{ return NULL; }

void
set_error_stream(int fd)
{
  error_fd = fd;

  error_write = write_raw;
}

void
set_error_raw(int raw)
{
  /* If the tty is set to raw mode, and we use the same tty for error
   * messages, we need to send some extra \r characters. */
     
  if (raw && (error_fd == STDERR_FILENO) && isatty(STDERR_FILENO))
    error_raw = 1;
  else
    error_raw = 0;
}
    
int
dup_error_stream(void)
{
  if (error_fd < 0)
    /* We're not writing error messages on any file; there's no
     * problem. */
    return 1;
  else
    {
      int fd = dup(error_fd);

      /* This function is used to get stderr away from the stdio fd
       * range. In the unlikely event that dup returns an fd <=
       * STDERR_FILENO, we treat that as an error. */
      if (fd > STDERR_FILENO)
	{
	  io_set_close_on_exec(fd);
	  error_fd = fd;
	  return 1;
	}

      if (fd >= 0)
	close(fd);
      
      return 0;
    }
}

void
set_error_ignore(void)
{
  error_write = write_ignore;
  error_fd = -1;
}

#define WERROR(l, d) (error_write(error_fd, (l), (d)))

static void
werror_flush(void)
{
  if (error_pos)
    {
      WERROR(error_pos, error_buffer);
      error_pos = 0;
    }
}

static void
werror_putc(uint8_t c)
{
  if (error_raw && c == '\n')
    /* We need a carriage return first. */
    werror_putc('\r');
  
  if (error_pos == BUF_SIZE)
    werror_flush();

  error_buffer[error_pos++] = c;
}

static void
werror_write_raw(uint32_t length, const uint8_t *msg)
{
  if (error_pos + length <= BUF_SIZE)
    {
      memcpy(error_buffer + error_pos, msg, length);
      error_pos += length;
    }
  else
    {
      werror_flush();
      WERROR(length, msg);
    }
}

static void
werror_write(uint32_t length, const uint8_t *msg)
{
  if (error_raw)
    {
      const uint8_t *eol;
      while ((eol = memchr(msg, '\n', length)))
	{
	  werror_write_raw(eol - msg, msg);
	  werror_putc('\n');

	  eol++;
	  length -= (eol - msg);
	  msg = eol;
	}
    }
  werror_write_raw(length, msg);
}

static void
werror_cstring(char *s) { werror_write(strlen(s), s); }

static void
werror_bignum(mpz_t n, int base)
{
  char *s = alloca(mpz_sizeinbase(n, base) + 2);
  mpz_get_str(s, 16, n);

  werror_cstring(s);
}

static void
werror_decimal(uint32_t n)
{
  unsigned length = format_size_in_decimal(n);
  uint32_t e = 1;
  unsigned i;

  /* An inefficient way of computing e = 10^(length - 1) */
  for (i = 1; i<length; i++)
    e *= 10;
  
  for (; e; e /= 10)
    {
      uint32_t digit = n / e;
      n = n % e;
      assert(digit < 10);
      werror_putc("0123456789"[digit]);
    }
}

static unsigned format_size_in_hex(uint32_t n);

static void
werror_hex_digit(unsigned digit)
{
  werror_putc("0123456789abcdef"[digit]);
}

static void
werror_hex_putc(uint8_t c)
{
  werror_hex_digit(c / 16);
  werror_hex_digit(c % 16);
}

static void
werror_hex(uint32_t n)
{
  unsigned left = 8;
  
  while ( (left > 1)
	  && !(n & 0xf0000000UL))
    {
      left --;
      n <<= 4;
    }
		    
  while (left--)
    {
      werror_hex_digit((n >> 28) & 0xf);
      n <<= 4;
    }
}

static void
werror_hexdump(uint32_t length, const uint8_t *data)
{
  uint32_t i = 0;
  
  werror("(size %i = 0x%xi)\n", length, length);

  for (i = 0; i<length; i+= 16)
    {
      unsigned j = format_size_in_hex(i);
      unsigned r = length - i;
      
      for ( ; j < 8; j++)
	werror_putc('0');

      werror_hex(i);
      werror_cstring(": ");

      if (r > 16)
	r = 16;

      for (j = 0; j<r; j++)
	werror_hex_putc(data[i+j]);

      for (; j<17; j++)
	werror_cstring("  ");

      for (j = 0; j<r; j++)
	{
	  uint8_t c = data[i+j];
	  if ( (c < 32) || (c > 126) )
	    c = '.';
	  werror_putc(c);
	}

      werror_cstring("\n");
    }
}

static void
werror_paranoia_putc(uint8_t c)
{
  switch (c)
    {
    case '\\':
      werror_cstring("\\\\");
      break;
    case '\r':
      /* Ignore */
      break;
    default:
      if (!isprint(c))
	{
	  werror_putc('\\');
	  werror_hex_putc(c);
	  break;
	}
      /* Fall through */
    case '\n':
      werror_putc(c);
      break;
    }
}

void
werror_vformat(const char *f, va_list args)
{
  if (program_name)
    {
      werror_write(strlen(program_name), program_name);
      werror_write(2, ": ");
    }
  
  while (*f)
    {
      if (*f == '%')
	{
	  int do_hex = 0;
	  int do_free = 0;
	  int do_paranoia = 0;
	  int do_utf8 = 0;

	  while (*++f)
	    switch (*f)
	      {
	      case 'x':
		do_hex = 1;
		break;
	      case 'f':
		do_free = 1;
		break;
	      case 'p':
		do_paranoia = 1;
		break;
	      case 'u':
		do_utf8 = 1;
		break;
	      default:
		goto end_options;
	      }
	end_options:
	  switch(*f++)
	    {
	    case '%':
	      werror_putc(*f);
	      break;
	    case 'i':
	      (do_hex ? werror_hex : werror_decimal)(va_arg(args, uint32_t));
	      break;
	    case 'c':
	      (do_paranoia ? werror_paranoia_putc : werror_putc)(va_arg(args, int));
	      break;
	    case 'n':
	      werror_bignum(va_arg(args, MP_INT *), do_hex ? 16 : 10);
	      break;
	    case 'a':
	      {
		int atom = va_arg(args, int);

		if (atom)
		  werror_write(get_atom_length(atom), get_atom_name(atom));
		else
		  werror_write(9, "<unknown>");
		break;
	      }
	    case 's':
	      {
		uint32_t length = va_arg(args, uint32_t);
		const uint8_t *s = va_arg(args, const uint8_t *);

		struct lsh_string *u = NULL; 

		if (do_utf8)
		  {
		    enum utf8_flag flags = utf8_replace;
		    if (do_paranoia)
		      flags |= utf8_paranoid;
		    
		    u = low_utf8_to_local(length, s, flags);
		    if (!u)
		      {
			werror_cstring("<Invalid utf-8 string>");
			break;
		      }
		    length = lsh_string_length(u);
		    s = lsh_string_data(u);
		  }
		if (do_hex)
		  {
		    assert(!do_paranoia);
		    werror_hexdump(length, s);
		  }
		else if (do_paranoia)
		  {
		    uint32_t i;
		    for (i=0; i<length; i++)
		      werror_paranoia_putc(*s++);
		  }
		else
		  werror_write(length, s);

		if (u)
		  lsh_string_free(u);
	      }
	      break;
	    case 'S':
	      {
		struct lsh_string *s = va_arg(args, struct lsh_string *);

		if (do_utf8)
		  {
		    enum utf8_flag flags = utf8_replace;
		    if (do_paranoia)
		      flags |= utf8_paranoid;

		    s = utf8_to_local(s, flags, do_free);
		    if (!s)
		      {
			werror_cstring("<Invalid utf-8 string>");
			break;
		      }
		    do_free = 1;
		  }
		if (do_hex)
		  {
		    assert(!do_paranoia);
		    werror_hexdump(STRING_LD(s));
		  }
		else if (do_paranoia)
		  {
		    uint32_t length = lsh_string_length(s);
		    const uint8_t *data = lsh_string_data(s);
		    uint32_t i;

		    for (i=0; i<length; i++)
		      werror_paranoia_putc(data[i]);
		  }
		else
		  werror_write(STRING_LD(s));

		if (do_free)
		  lsh_string_free(s);

		break;
	      }
	    case 't':
	      {
		struct lsh_object *o = va_arg(args, struct lsh_object *);
		const char *type;

		if (!o)
		  type = "<NULL>";
		else if (o->isa)
		  type = o->isa->name;
		else
		  type = "<STATIC>";

		werror_write(strlen(type), type);

		break;
	      }
	    case 'z':
	      {
		char *s = va_arg(args, char *);

		if (do_hex)
		  werror_hexdump(strlen(s), s);

		else if (do_paranoia)
		  while (*s)
		    werror_paranoia_putc(*s++);
		else
		  werror_write(strlen(s), s);
		
		break;
	      }
	    case 'e':
	      { /* errno specifier */
		int e = va_arg(args, int);
		werror_cstring("(errno = ");
		werror_decimal(e);
		werror_cstring("): ");
		werror_cstring(STRERROR(errno));

		break;
	      }
	    default:
	      fatal("werror_vformat: bad format string!\n");
	      break;
	    }
	}
      else
	werror_putc(*f++);
    }
}

/* Unconditionally display message. */
static void
werror_format(const char *format, ...) 
{
  va_list args;

  va_start(args, format);
  werror_vformat(format, args);
  va_end(args);
  werror_flush();
}

void
werror(const char *format, ...) 
{
  va_list args;

  /* It is somewhat reasonable to use both -q and -v. In this case
   * werror()-messages should be displayed. */
  if (verbose_flag || !quiet_flag)
    {
      va_start(args, format);
      werror_vformat(format, args);
      va_end(args);
      werror_flush();
    }
}

void
werror_progress(const char *string)
{
  if (verbose_flag || !quiet_flag)
    {
      werror_write(strlen(string), string);
      werror_flush();
    }
}

void
trace(const char *format, ...) 
{
  va_list args;

  if (trace_flag)
    {
      va_start(args, format);
      werror_vformat(format, args);
      va_end(args);
      werror_flush();
    }
}

void
debug(const char *format, ...) 
{
  va_list args;

  if (debug_flag)
    {
      va_start(args, format);
      werror_vformat(format, args);
      va_end(args);
      werror_flush();
    }
}

void
verbose(const char *format, ...) 
{
  va_list args;

  if (verbose_flag)
    {
      va_start(args, format);
      werror_vformat(format, args);
      va_end(args);
      werror_flush();
    }
}

#define FATAL_SLEEP 0
#ifndef FATAL_SLEEP
# define FATAL_SLEEP 0
#endif

void
fatal(const char *format, ...) 
{
  va_list args;

  va_start(args, format);
  werror_vformat(format, args);
  va_end(args);
  werror_flush();

#if FATAL_SLEEP
  werror_format("attach gdb to process %i. Going to sleep...\n", getpid());
  for (;;)
    sleep(4711);
#endif
#if WITH_GCOV
  /* We want the process to exit, so that it writes profiling data,
     but we also want it to dump core. So let's fork. */
  if (fork())
    /* Let the parent process exit (we also exit if fork fails, but
       that case doesn't really matter here) */
    exit(255);
#endif
  
  abort();
}

static unsigned
format_size_in_hex(uint32_t n)
{
  int i;
  int e;
  
  /* Table of 16^(2^n) */
  static const uint32_t powers[] = { 0x10UL, 0x100UL, 0x10000UL };

#define SIZE (sizeof(powers) / sizeof(powers[0])) 

  /* Determine the smallest e such that n < 16^e */
  for (i = SIZE - 1 , e = 0; i >= 0; i--)
    {
      if (n >= powers[i])
	{
	  e += 1UL << i;
	  n /= powers[i];
	}
    }

#undef SIZE
  
  return e+1;
}

