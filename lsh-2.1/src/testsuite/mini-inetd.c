/* mini-inetd
 *
 * This program is an total reimplementation of the old mini-inetd
 * from Thomas Bellman's tcputils.
 *
 * Copyright (C) 2010 Möller
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/* For IPPROTO_TCP on FreeBSD */
#include <netinet/in.h>

#include "getopt.h"

static void NORETURN PRINTF_STYLE(1, 2)
die(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  exit(EXIT_FAILURE);
}

static void PRINTF_STYLE(1, 2)
werror(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

static void
usage (FILE *f)
{
  fprintf(f,
	  /* FIXME: ':' is a bad separator for literal IPv6 addresses.
	     Support [] around the address? */
	  "mini-inetd [OPTIONS] [localaddr:]port program [argv0, argv1 ...]\n"
	  "Options:\n"
	  "  -m max-connections\n"
	  "  --help\n"
	  "  --background\n"
	  "  -4     Only use IPv4.\n"
	  "  -6     Only use IPv6.\n"
	  "  --help Display this help.\n");
}

static void
start_service (int fd, const char *program, char **argv)
{
  int pid = fork();
  if (pid < 0)
    die ("fork failed: %s\n", STRERROR(errno));
  if (pid)
    {
      /* parent */
      close(fd);
    }
  else
    {
      /* child */
      dup2(fd, STDIN_FILENO);
      dup2(fd, STDOUT_FILENO);
      close(fd);

      execv(program, argv);
      werror("execv failed: %s\n", STRERROR(errno));
      _exit(EXIT_FAILURE);
    }  
}

#define MAX_SOCKETS 7

int
main (int argc, char **argv)
{
  int max_connections = 0;
  int background = 0;
  int family = AF_UNSPEC; 
  const char *local_addr;
  const char *port;
  int c;
  char *sep;

  struct addrinfo hints;
  struct addrinfo *list, *p;
  int err;
  
  int fds[MAX_SOCKETS];
  int nfds = 0;
  int max_fd = 0;

  enum { OPT_HELP = 300, OPT_BACKGROUND };
  static const struct option options[] =
    {
      /* Name, args, flag, val */

      { "help", no_argument, NULL, OPT_HELP },
      { "background", no_argument, NULL, OPT_BACKGROUND },
      { NULL, 0, NULL, 0 }
    };  

  while ( (c = getopt_long (argc, argv, "46dm:", options, NULL)) != -1)
    switch (c)
      {
      case '?':
	return EXIT_FAILURE;
      case OPT_HELP:
 	usage(stdout);
	return EXIT_SUCCESS;
      case OPT_BACKGROUND:
	background = 1;
	break;
      case 'd':
	/* Currently a NOP for backwards compatibility. */
	break;
      case 'm':
	max_connections = atoi(optarg);
	if (max_connections <= 0)
	  die ("Invalid argument for -m, should be a positive integer.\n");

	break;
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
      }

  argc -= optind;
  argv += optind;
  
  if (argc < 2)
    {
      usage(stderr);
      return EXIT_FAILURE;
    }
  sep = strrchr(argv[0], ':');
  if (sep)
    {
      port = sep + 1;
      *sep = '\0';
      local_addr = argv[0];
    }
  else
    {
      port = argv[0];
      local_addr = NULL;
    }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = family;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (!local_addr)
    hints.ai_flags = AI_PASSIVE;

  err = getaddrinfo (local_addr, port, &hints, &list);
  if (err)
    die("Failed to look up address: %s\n", gai_strerror(err));

  for (p = list; p; p = p->ai_next)
    {
      int yes = 1;
      int fd;
      int flags;
      
      if (nfds == MAX_SOCKETS)
	{
	  werror("Large number of matching addresses!? Ignoring all but the first %d\n",
		 MAX_SOCKETS);
	  break;
	}
      fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (fd < 0)
	{
	  werror("Failed to create socket of family %d\n",
		 p->ai_family);
	  continue;
	}
      
      if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) <0)
	werror("setsockopt SO_REUSEADDR failed: %s\n", STRERROR(errno));

#if WITH_IPV6 && defined (IPV6_V6ONLY)
      if (p->ai_family == AF_INET6)
	{
	  if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes)) < 0)
	    werror("setsockopt IPV6_V6ONLY failed: %s\n", STRERROR(errno));
	}
#endif

      flags = fcntl(fd, F_GETFL);
      if (flags < 0)
	werror("fcntl F_GETFL failed: %s\n", STRERROR(errno));
      else if (fcntl(fd, F_SETFL, flags | 1 | O_NONBLOCK) < 0)
	werror("fcntl F_SETFL failed: %s\n", STRERROR(errno));

      if (bind(fd, p->ai_addr, p->ai_addrlen) < 0)
	{
	  werror("bind failed: %s\n", STRERROR(errno));
	  close(fd);
	  continue;
	}

      if (listen (fd, 256) < 0)
	{
	  werror("listen failed: %s\n", STRERROR(errno));
	  close(fd);
	  continue;
	}
      fds[nfds++] = fd;
      if (fd > max_fd)
	max_fd = fd;      
    }
  freeaddrinfo(list);

  if (!nfds)
    die("No ports could be bound.\n");

  if (background)
    {
      pid_t pid = fork();
      switch (pid)
	{
	case -1:
	  die("fork failed: %s\n", STRERROR(errno));
	case 0:
	  /* Child process. */
	  break;
	default:
	  printf("%ld\n", (long) pid);
	  fflush(stdout);
	  _exit(EXIT_SUCCESS);
	}
    }
  for (;;)
    {
      fd_set wanted;
      int i;
      int res;

      FD_ZERO(&wanted);
      for (i = 0; i < nfds; i++)
	FD_SET(fds[i], &wanted);

      do
	res = select(max_fd + 1, &wanted, NULL, NULL, NULL);
      while (res < 0 && errno == EINTR);

      for (i = 0; i < nfds; i++)
	if (FD_ISSET(fds[i], &wanted))
	  {
	    int conn;
	    int flags;

	    conn = accept(fds[i], NULL, NULL);
	    if (conn < 0)
	      {
		if (errno != EINTR)
		  die("accept failed: %s\n", STRERROR(errno));
		continue;
	      }

	    /* With traditional BSD behavior, the acccepted socket
	       inherits the O_NONBLOCK flag from the listening socket.
	       So clear it explicitly. */
	    flags = fcntl(conn, F_GETFL);
	    if (flags < 0)
	      werror("fcntl F_GETFL failed: %s\n", STRERROR(errno));
	    else if (fcntl(conn, F_SETFL, flags & ~O_NONBLOCK) < 0)
	      werror("fcntl F_SETFL failed: %s\n", STRERROR(errno));

	    start_service (conn, argv[1], argv + 2);

	    if (max_connections)
	      {
		max_connections--;
		if (!max_connections)
		  return EXIT_SUCCESS;
	      }
	}
    }
}
