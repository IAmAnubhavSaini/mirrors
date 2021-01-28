/* socks.c
 *
 * References:
 *
 * Socks4 is described in http://archive.socks.permeo.com/protocol/socks4.protocol.
 * Socks5 is described in RFC 1928.
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2004 Niels Möller
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
#include <stdlib.h>
#include <string.h>

#include <nettle/macros.h>

#include "channel_forward.h"
#include "command.h"
#include "connection_commands.h"
#include "format.h"
#include "io_commands.h"
#include "lsh_string.h"
#include "resource.h"
#include "ssh.h"
#include "tcpforward.h"
#include "werror.h"
#include "xalloc.h"

/* Various protocol constants */
enum {
  /* Version exchange */
  SOCKS_NOAUTH = 0,
  SOCKS_NOMETHOD = 0xff,
};

/* Commands */
enum {
  SOCKS_CONNECT = 1,
  SOCKS_BIND = 2,
  SOCKS_UDP = 3,
};

/* Addresses */
enum {
  SOCKS_IP4 = 1,
  SOCKS_DNS = 3,
  SOCKS_IP6 = 4,
};

/* Status codes */
enum {
  SOCKS_ERROR_NONE = 0,
  SOCKS_ERROR_GENERAL = 1,
  SOCKS_ERROR_NOT_ALLOWED = 2,
  SOCKS_ERROR_NET_UNREACHABLE = 3,
  SOCKS_ERROR_HOST_UNREACHABLE = 4,
  SOCKS_ERROR_CONNECTION_REFUSED = 5,
  SOCKS_ERROR_TTL_EXPIRED = 6,
  SOCKS_ERROR_COMMAND = 7,
  SOCKS_ERROR_ADDRESS = 8,
};

/* Message sizes. Maximum size is a request with a 256 byte DNS
   address, 262 bytes in all */
enum {
  SOCKS_HEADER_SIZE = 2,
  SOCKS_COMMAND_SIZE = 5,
  SOCKS4_COMMAND_SIZE = 9, /* FIXME: For now we support only empty usernames */
  SOCKS_MAX_SIZE = 262,
};

enum socks_state {
  SOCKS_VERSION_HEADER, SOCKS_VERSION_METHODS,
  SOCKS_COMMAND_HEADER, SOCKS_COMMAND_ADDR,
  SOCKS4_COMMAND,
  SOCKS_COMMAND_WAIT,
};

struct command_2 socks_handshake;
#define SOCKS_HANDSHAKE (&socks_handshake.super.super)

#include "socks.c.x"

/* GABA:
   (class
     (name socks_connection)
     (vars
       (version . uint8_t)
       (connection object ssh_connection)
       (peer object listen_value)))
*/

static void
socks_close(struct socks_connection *self)
{
  close_fd_nicely(self->peer->fd);
}

static void
socks_fail(struct socks_connection *self)
{
  close_fd(self->peer->fd);
}

static void
socks_write(struct socks_connection *self, struct lsh_string *data)
{
  A_WRITE(&self->peer->fd->write_buffer->super, data);
}
     
static void
socks_method(struct socks_connection *self, uint8_t method)
{
  socks_write(self, ssh_format("%c%c", self->version, method));
}

static void
socks_reply(struct socks_connection *self,
	    uint8_t status,
	    uint8_t atype,
	    uint32_t alength,
	    const uint8_t *addr,
	    uint16_t port)
{
  switch (self->version)
    {
    default:
      fatal("socks_reply: Internal error\n");
      
    case 5:
      socks_write(self, ssh_format("%c%c%c%c%ls%c%c",
				   self->version, status, 0, atype,
				   alength, addr,
				   port >> 8, port & 0xff));
      break;
    case 4:
      assert(atype == SOCKS_IP4);
      assert(alength == 4);

      socks_write(self, ssh_format("%c%c%c%c%ls",
				   0, status ? 91 : 90,
				   port >> 8, port & 0xff,
				   alength, addr));
      break;
    }
}

static struct address_info *
socks2address_info(uint8_t atype,
		   const uint8_t *addr,
		   uint16_t port)
{
  /* The type is checked earlier */
  struct lsh_string *host;
  
  switch (atype)
    {
    default:
      abort();
      
    case SOCKS_IP4:
      host = ssh_format("%di.%di.%di.%di", addr[0], addr[1], addr[2], addr[3]);
      break;
    case SOCKS_IP6:
      /* It's possible to support ipv6 targets without any native ipv6
	 support, but it's easier if we hafve standard functinos and
	 constants like AF_INET6 and inet_ntop. */
#if WITH_IPV6
      host = lsh_string_ntop(AF_INET6, INET6_ADDRSTRLEN, addr);
      break;
#else
      return NULL;
#endif
    case SOCKS_DNS:
      host = ssh_format("%ls", addr[0], addr + 1);
      break;
    }
  return make_address_info(host, port);
}

/* GABA:
   (class
     (name socks_continuation)
     (super command_continuation)
     (vars
       (socks object socks_connection)))
*/

static void
do_socks_continuation(struct command_continuation *s, struct lsh_object *x)
{
  CAST(socks_continuation, self, s);
  CAST_SUBTYPE(channel_forward, channel, x);

  static uint8_t noaddr[4] = {0,0,0,0};

  /* We don't have the address at the server's end, so we can't pass it along. */
  socks_reply(self->socks, SOCKS_ERROR_NONE, SOCKS_IP4, 4, noaddr, 0);

  /* Overwrites the read and close callbacks we have installed. */
  channel_forward_start_io_read(channel);
}

static struct command_continuation *
make_socks_continuation(struct socks_connection *socks)
{
  NEW(socks_continuation, self);
  self->super.c = do_socks_continuation;
  self->socks = socks;

  return &self->super;
}

/* GABA:
   (class
     (name socks_exception_handler)
     (super exception_handler)
     (vars
       (socks object socks_connection)))
*/

static void
do_exc_socks_handler(struct exception_handler *s,
		     const struct exception *e)
{
  CAST(socks_exception_handler, self, s);
  if (e->type & EXC_IO)
    {
      CAST_SUBTYPE(io_exception, exc, e);
      if (exc->fd)
	close_fd(exc->fd);

      werror("Socks: %z, (errno = %i)\n", e->msg, exc->error);
    }
  else if (e->type == EXC_CHANNEL_OPEN)
    {
      CAST_SUBTYPE(channel_open_exception, exc, e);
      uint8_t reply = SOCKS_ERROR_GENERAL;
      static uint8_t noaddr[4] = {0,0,0,0};
      
      if (exc->error_code == SSH_OPEN_ADMINISTRATIVELY_PROHIBITED)
	reply = SOCKS_ERROR_NOT_ALLOWED;
      else if (exc->error_code == SSH_OPEN_CONNECT_FAILED)
	reply = SOCKS_ERROR_CONNECTION_REFUSED;

      verbose("Socks forwarding denied by server: %z\n", e->msg);
      socks_reply(self->socks, reply, SOCKS_IP4, 4, noaddr, 0);
    }
  else
    EXCEPTION_RAISE(self->super.parent, e);
}

static struct exception_handler *
make_socks_exception_handler(struct socks_connection *socks,
			     struct exception_handler *e,
			     const char *context)
{
  NEW(socks_exception_handler, self);
  self->super.parent = e;
  self->super.raise = do_exc_socks_handler;
  self->super.context = context;

  self->socks = socks;
  return &self->super;
}

static int
socks_command(struct socks_connection *self, uint8_t command,
	      uint8_t addr_type, const uint8_t *addr,
	      uint16_t port)
{
  static const uint8_t noaddr[4] = {0,0,0,0};  
  if (command != SOCKS_CONNECT)
    {
      socks_reply(self, SOCKS_ERROR_COMMAND, SOCKS_IP4, sizeof(noaddr), noaddr, 0);
      return 0;
    }
  else
    {
      struct address_info *target = socks2address_info(addr_type, addr, port);
      struct command *open_command;
      
      if (!target)
	{
	  socks_reply(self, SOCKS_ERROR_ADDRESS, SOCKS_IP4, sizeof(noaddr), noaddr, 0); 
	  return 0;
	}

      open_command = make_open_tcpip_command(ATOM_DIRECT_TCPIP, target, self->peer);
      COMMAND_CALL(open_command,
		   self->connection,
		   make_socks_continuation(self),
		   make_socks_exception_handler(self, &default_exception_handler,
						HANDLER_CONTEXT));
      return 1;
    }
}

/* GABA:
   (class
     (name read_socks)
     (super read_handler)
     (vars
       (socks object socks_connection)
       (buffer string)
       (pos . uint32_t)
       (state . "enum socks_state")
       (length . uint32_t)))
*/

static uint32_t
do_read_socks(struct read_handler **h,
	      uint32_t available,
	      const uint8_t *data)
{
  CAST(read_socks, self, *h);
  const uint8_t *p;

  assert(self->buffer);
  
  if (!available)
    {
      socks_close(self->socks);
      
      *h = NULL;
      return 0;
    }

  if (self->length - self->pos > available)
    {
      lsh_string_write(self->buffer, self->pos, available, data);
      self->pos += available;
      return available;
    }

  available = self->length - self->pos;
  lsh_string_write(self->buffer, self->pos, available, data);
  self->pos = self->length;

  p = lsh_string_data(self->buffer);
  
  switch (self->state)
    {
    default:
      abort();

      /* For socks 4, the command is sent directly,

         byte     version ; 4
	 byte     command
	 uint16   port
	 uint32   ip
	 byte[n]  NUL-terminated userid	 
      */
      
      /* For socks 5, the initial version exchange is:

         byte     version ; 5
	 byte     n; >= 1
	 byte[n]  methods
      */
      
    case SOCKS_VERSION_HEADER:
      self->socks->version = p[0];
      verbose("Socks version %i connection.\n", self->socks->version);

      switch (self->socks->version)
	{
	default:
	  werror("Socks connection of unknown version %i.\n", p[0]);
	  socks_fail(self->socks);
	  break;
	  
	case 4:
	  self->length = SOCKS4_COMMAND_SIZE;
	  self->state = SOCKS4_COMMAND;
	  break;
	  
	case 5:
	  self->length = 2 + p[1];
	  self->state = SOCKS_VERSION_METHODS;
	  break;
	}
      break;
      
    case SOCKS_VERSION_METHODS:
      /* We support only method 0 */
      if (memchr(p+2, SOCKS_NOAUTH, p[1]))
	{
	  socks_method(self->socks, SOCKS_NOAUTH);
	  
	  self->pos = 0;
	  self->length = SOCKS_COMMAND_SIZE;
	  self->state = SOCKS_COMMAND_HEADER;
	}
      else
	{
	  werror("Socks client doesn't support no authentication!?\n");
	  socks_method(self->socks, SOCKS_NOMETHOD);
	  socks_close(self->socks);
	}
      break;

    case SOCKS_COMMAND_HEADER:
      /* A request has the syntax
	 byte    version
	 byte    command
	 byte    reserved ; 0
	 byte    atype
	 byte[n] address
	 uint16  port     ; network byte order

	 atype : n

	     1 : 4  (ipv4 address, network? byte order)
	     3 : 1 + first byte of address
	     4 : 16 (ipv6 address)

	 We count the first byte of address as part of the header.
      */
      if (p[0] != self->socks->version || p[2] != 0)
	{
	  werror("Invalid socks request.\n");
	  socks_fail(self->socks);
	}
      else
	{
	  self->state = SOCKS_COMMAND_ADDR;
	  
	  switch (p[3])
	    {
	    case SOCKS_IP4:
	      self->length = 10;
	      break;
	    case SOCKS_IP6:
	      self->length = 22;
	      break;
	    case SOCKS_DNS:
	      if (p[4] == 0)
		socks_fail(self->socks);
	      else
		self->length = 7 +  p[4];
	    }
	}
      break;

    case SOCKS_COMMAND_ADDR:
      if (socks_command(self->socks, p[1], p[3], p+4,
			READ_UINT16(p + self->length - 2)))
	{
	  self->state = SOCKS_COMMAND_WAIT;
	}
      else
	{
	  socks_fail(self->socks);
	}
      
      lsh_string_free(self->buffer);
      self->buffer = NULL;
      *h = NULL;
      break;
      
    case SOCKS4_COMMAND:
      if (p[SOCKS4_COMMAND_SIZE - 1] != 0)
	/* FIXME: We should read and ignore the user name. If we are
	 * lucky, it's already in the i/o buffer and will be
	 * discarded. */
	werror("Socks 4 usernames not yet supported. May or may not work.\n");
      
      if (socks_command(self->socks, p[1], SOCKS_IP4, p+4,
			READ_UINT16(p + 2)))
	{
	  self->state = SOCKS_COMMAND_WAIT;
	}
      else
	{
	  socks_fail(self->socks);
	}

      lsh_string_free(self->buffer);
      self->buffer = NULL;
      *h = NULL;
      break;
    }
  
  return available;
}
  
static struct read_handler *
make_read_socks(struct socks_connection *socks)
{
  NEW(read_socks, self);

  self->super.handler = do_read_socks;
  self->socks = socks;
  self->buffer = lsh_string_alloc(SOCKS_MAX_SIZE);
  self->pos = 0;
  self->state = SOCKS_VERSION_HEADER;
  self->length = SOCKS_HEADER_SIZE;

  return &self->super;
}

/* The read buffer is replaced when we go into connected mode, but the
   writebuffer is not */
#define SOCKS_READ_BUF_SIZE 100
#define SOCKS_WRITE_BUF_SIZE (SSH_MAX_PACKET * 10)

/* (socks_handshake peer connection) */
DEFINE_COMMAND2(socks_handshake)
     (struct command_2 *s UNUSED,
      struct lsh_object *a1,
      struct lsh_object *a2,
      struct command_continuation *c,
      struct exception_handler *e UNUSED)
{
  CAST(listen_value, peer, a1);
  CAST(ssh_connection, connection, a2);
  NEW(socks_connection, self);
  
  self->connection = connection;
  self->peer = peer;
  remember_resource(connection->resources, &peer->fd->super);

  io_read_write(peer->fd, make_buffered_read(SOCKS_READ_BUF_SIZE, make_read_socks(self)),
		SOCKS_WRITE_BUF_SIZE, NULL);

  COMMAND_RETURN(c, self);
}

/* GABA:
   (expr
     (name forward_socks)
     (params
       (local object address_info))
     (expr
       (lambda (connection)
         (connection_remember connection
           (listen
	     (lambda (peer)
	       (socks_handshake peer connection))
	     ; NOTE: The use of prog1 is needed to delay the bind call
	     ; until the (otherwise ignored) connection argument is
	     ; available.
	     (bind (prog1 local connection)))))))
*/

struct command *
make_socks_server(struct address_info *local)
{
  return forward_socks(local);
}
