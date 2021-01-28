/* tcpconnect
 *
 * This program is an total reimplementation of the old tcpconnect
 * from Thomas Bellman's tcputils.
 *
 * Copyright (C) 2012 Möller
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301  USA
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>

/* For IPPROTO_TCP on FreeBSD */
#include <netinet/in.h>

#include <arpa/inet.h>

#include "getopt.h"

static void NORETURN PRINTF_STYLE(1, 2)
die(const char *format, ...)
{
  va_list args;
  fprintf(stderr, "tcpconnect: ");
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  exit(EXIT_FAILURE);
}

static void PRINTF_STYLE(1, 2)
werror(const char *format, ...)
{
  va_list args;
  fprintf(stderr, "tcpconnect: ");
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

static void
usage (FILE *f)
{
  fprintf(f,
	  "tcpconnect [OPTIONS] host post\n"
	  "Options:\n"
	  "  -i     Terminate at end-of-file on standard input;\n"
	  "         don't wait for the server to close the connection.\n"
	  "  -r     Terminate when the remote server closes the connection;\n"
	  "         don't wait for end-of-file on standard input.\n"
	  "  -v     Verbose mode. Prints a message to standard error when\n"
	  "         the connection has been established.\n"
	  /* FIXME: ':' is a bad separator for literal IPv6 addresses.
	     Support [] around the address? */
	  "  -l ADDR:PORT\n"
	  "         Bind the local end-point of the connection to IP address\n"
	  "         ADDR, TCP port PORT. Either the IP address or the port,\n"
	  "         but not both, may be left out, meaning that the operating\n"
	  "         system gets to choose that part by itself.\n"
	  "  -4     Only use IPv4.\n"
	  "  -6     Only use IPv6.\n"
	  "  --help Display this help.\n");
}

#define BUF_SIZE 4096

int
main (int argc, char **argv)
{
  int family = AF_UNSPEC;
  int wait_stdin_eof = 1;
  int wait_remote_eof = 1;
  int verbose = 0;
#if 0
  const char *local_ip = NULL;
  const char *local_port = NULL;
#endif
  int c;

  struct addrinfo hints;
  struct addrinfo *list, *p;
  int flags;
  int fd;

  int seen_remote_eof;
  int seen_stdin_eof;

  char buf_stdin[BUF_SIZE];
  unsigned buf_size;
  unsigned buf_pos;
  int err;

  enum { OPT_HELP = 300 };
  static const struct option options[] =
    {
      /* Name, args, flag, val */

      { "help", no_argument, NULL, OPT_HELP },
      { NULL, 0, NULL, 0 }
    };  

  while ( (c = getopt_long (argc, argv, "46irvl:", options, NULL)) != -1)
    switch (c)
      {
      case '?':
	return EXIT_FAILURE;
      case OPT_HELP:
 	usage(stdout);
	return EXIT_SUCCESS;
      case '4':
	family = AF_INET;
	break;
      case '6':
#ifdef AF_INET6
	family = AF_INET6;
	break;
#else
	die ("IP version 6 not supported.\n");
#endif
      case 'i':
	wait_remote_eof = 0;
	break;	
      case 'r':
	wait_stdin_eof = 0;
	break;
      case 'v':
	verbose = 1;
	break;
      case 'l':
	die ("Not implemented.\n");
	break;
      }

  argc -= optind;
  argv += optind;
  
  if (argc < 2)
    {
      usage(stderr);
      return EXIT_FAILURE;
    }
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = family;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  err = getaddrinfo (argv[0], argv[1], &hints, &list);
  if (err)
    die("Failed to look up address: %s\n", gai_strerror(err));

  for (p = list, fd = -1; p; p = p->ai_next)
    {
      fd = socket (p->ai_family, p->ai_socktype, p->ai_protocol);
      if (fd < 0)
	continue;
      if (connect (fd, p->ai_addr, p->ai_addrlen) >= 0)
	break;

      if (verbose)
	fprintf(stderr, "Connect failed: %s\n", strerror(errno));

      close (fd);
      fd = -1;
    }

  if (fd < 0)
    die("Connection failed.\n");
  
  if (verbose)
    switch (p->ai_family)
      {
#ifdef AF_INET6
      case AF_INET6:
	{
	  const struct sockaddr_in6 *sin6
	    = (struct sockaddr_in6 *) p->ai_addr;
	    
	  char buf[INET6_ADDRSTRLEN];
	  if (!inet_ntop (p->ai_family, &sin6->sin6_addr, buf, sizeof(buf)))
	    die ("inet_ntop failed: %s\n", strerror(errno));
	  fprintf(stderr, "[Connected to %s port %d]\n",
		  buf, ntohs (sin6->sin6_port));
	  break;
	}
#endif
      case AF_INET:
	{
	  const struct sockaddr_in *sin
	    = (struct sockaddr_in *) p->ai_addr;
	    
	  char buf[INET_ADDRSTRLEN];
	  if (!inet_ntop (p->ai_family, &sin->sin_addr, buf, sizeof(buf)))
	    die ("inet_ntop failed: %s\n", strerror(errno));
	  fprintf(stderr, "[Connected to %s port %d]\n",
		  buf, ntohs (sin->sin_port));
	  break;
	}
      default:
	die ("Unknown address family %d.\n", p->ai_family);
      }
  freeaddrinfo (list);

  flags = fcntl(fd, F_GETFL);
  if (flags < 0)
    werror("fcntl F_GETFL failed: %s\n", strerror(errno));
  else if (fcntl(fd, F_SETFL, flags | 1 | O_NONBLOCK) < 0)
    werror("fcntl F_SETFL failed: %s\n", strerror(errno));

  signal(SIGPIPE, SIG_IGN);

  buf_size = buf_pos = 0;  
  seen_stdin_eof = seen_remote_eof = 0;

  /* We do blocking reads of stdin and writes to stdout, and use
     FIONREAD to avoid blocking on read. We do non-blocking reads and
     writes on the sockets. If we can't write, we keep buffered data
     in buf_stdin, and don't read any more stdin data until the
     buffered data has been written to the socket. */
  for (;;)
    {
      fd_set read_fds;
      fd_set write_fds;

      FD_ZERO (&read_fds);
      FD_ZERO (&write_fds);

      if (buf_size > buf_pos)
	FD_SET (fd, &write_fds);
      else if (!seen_stdin_eof)
	FD_SET (STDIN_FILENO, &read_fds);

      if (!seen_remote_eof)
	FD_SET (fd, &read_fds);

      if (select (fd + 1, &read_fds, &write_fds, NULL, NULL) < 0)
	{
	  if (errno == EINTR)
	    continue;
	  die ("select failed: %s\n", strerror(errno));
	}
      if (FD_ISSET (fd, &write_fds))
	{
	  int res = write(fd, buf_stdin + buf_pos, buf_size - buf_pos);
	  if (res < 0)
	    {
	      if (errno != EINTR)
		{
		  if (verbose)
		    werror ("write to socket failed: %s\n", strerror(errno));
		  break;
		}
	    }
	  else
	    buf_pos += res;
	}
      if (FD_ISSET (STDIN_FILENO, &read_fds))
	{	  
	  int nbytes, res;

#ifdef FIONREAD
	  if (ioctl (STDIN_FILENO, FIONREAD, &nbytes) < 0
	      || nbytes <= 0
	      || nbytes > (int) sizeof(buf_stdin))
#endif
	    nbytes = sizeof(buf_stdin);

	  res = read(STDIN_FILENO, buf_stdin, nbytes);
	  if (res < 0)
	    {
	      if (errno != EINTR)
		die("read from stdin failed: %s\n", strerror(errno));
	    }
	  else if (res == 0)
	    {
	      seen_stdin_eof = 1;
	      wait_stdin_eof = 0;
	      if (!wait_remote_eof)
		break;
	      if (shutdown (fd, SHUT_WR) < 0 && errno != ENOTCONN)
		die("shutdown failed: %s\n", strerror(errno));
	    }
	  else
	    {	      
	      nbytes = res;	      
	      res = write(fd, buf_stdin, nbytes);
	      if (res < 0)
		{
		  if (errno != EINTR)
		    {
		      if (verbose)
			werror ("write to socket failed: %s\n", strerror(errno));
		      break;
		    }
		  res = 0;
		}
	      if (res < nbytes)
		{
		  buf_size = nbytes;
		  buf_pos = res;
		}
	    } 
	}
      if (FD_ISSET (fd, &read_fds))
	{
	  char buf_remote[BUF_SIZE];
	  int res = read (fd, buf_remote, sizeof (buf_remote));

	  if (res <= 0)
	    {
	      /* Treat read errors in the same way as EOF. */
	      if (res < 0 && (errno == EINTR || errno == EWOULDBLOCK))
		; /* Do nothing */
	      else
		{
		  if (verbose && res < 0)
		    werror("read from socket failed: %s\n", strerror(errno));

		  seen_remote_eof = 1;
		  wait_remote_eof = 0;
		  if (!wait_stdin_eof)
		    break;

		  /* Useful only in case stdout happens to be a socket. */
		  shutdown (STDOUT_FILENO, SHUT_WR);
		}
	    }
	  else
	    {
	      int size = res;
	      int done = 0;
	      while (done < size)
		{
		  res = write(STDOUT_FILENO, buf_remote + done, size - done);
		  if (res < 0)
		    {
		      if (errno != EINTR)
			die ("write to stdout failed: %s\n", strerror(errno));
		    }
		  else
		    done += res;
		}
	    }
	}
    }
  close (fd);

  if (verbose)
    fprintf(stderr, "[Connection closed]\n");

  return EXIT_SUCCESS;
}
