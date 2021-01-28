/* gateway_commands.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2000 Niels Möller
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
#include <string.h>

#include <nettle/macros.h>

#include "gateway_commands.h"

#include "channel.h"
#include "connection_commands.h"
#include "format.h"
#include "gateway_channel.h"
#include "io_commands.h"
#include "lsh_string.h"
#include "read_packet.h"
#include "ssh.h"
#include "werror.h"
#include "xalloc.h"

#define HEADER_SIZE 4
#include "gateway_commands.c.x"

/* The protocol used over the gateway socket is clear text ssh-packets
 *
 *   uint32     packet_length
 *   byte[n]    payload; n = packet_length
 *
 * There is no extra padding. Max packet size
 * connection->rec_max_packet.*/

static void
do_gateway_pad(struct abstract_write *w,
	       struct lsh_string *packet)
{
  CAST(abstract_write_pipe, self, w);

  A_WRITE(self->next, ssh_format("%fS", packet));
}

static struct abstract_write *
make_gateway_pad(struct abstract_write *next)
{
  NEW(abstract_write_pipe, self);

  self->super.write = do_gateway_pad;
  self->next = next;

  return &self->super;
}

/* GABA:
   (class
     (name read_gateway_packet)
     (super read_handler)
     (vars
       ; Buffer index, used for all the buffers
       (pos . uint32_t)
       (header string)
       (payload string)
       (handler object abstract_write)
       (connection object ssh_connection)))
*/

static uint32_t
do_read_gateway(struct read_handler **h,
		uint32_t available,
		const uint8_t *data)
{
  CAST(read_gateway_packet, self, *h);

  if (!available)
    {
      debug("read_gateway: Got EOF.\n");

      if (self->payload)
        EXCEPTION_RAISE(self->connection->e,
                        make_protocol_exception(0, "Unexpected EOF"));
      else
        EXCEPTION_RAISE(self->connection->e, &finish_read_exception);

      *h = NULL;
      return 0;
    }

  if (!self->payload)
    {
      /* Get length field */	
      uint32_t left;
      uint32_t length;
	
      assert(self->pos < HEADER_SIZE);	
      left = HEADER_SIZE - self->pos;

      if (available < left)
	{
	  lsh_string_write(self->header, self->pos, available, data);
	  self->pos += available;
	  return available;
	}

      lsh_string_write(self->header, self->pos, left, data);

      length = READ_UINT32(lsh_string_data(self->header));
      if (length > self->connection->rec_max_packet)
	{
	  static const struct protocol_exception too_large =
	    STATIC_PROTOCOL_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
				      "Packet too large");
	    
	  werror("read_gateway: Receiving too large packet.\n"
		 "  %i octets, limit is %i\n",
		 length, self->connection->rec_max_packet);
	    
	  EXCEPTION_RAISE(self->connection->e, &too_large.super);
	  *h = NULL;
	}
      else
	{
	  self->payload = lsh_string_alloc(length);
	  self->pos = 0;
	}

      return left;
    }
  else
    {
      uint32_t left;
      left = lsh_string_length(self->payload) - self->pos;

      if (available < left)
	{
	  lsh_string_write(self->payload, self->pos, available, data);
	  self->pos += available;
	  return available;
	}

      lsh_string_write(self->payload, self->pos, left, data);

      A_WRITE(self->handler, self->payload);
      self->payload = NULL;
      self->pos = 0;

      return left;
    }
}

static struct read_handler *
make_read_gateway(struct abstract_write *handler,
		  struct ssh_connection *connection)
{
  NEW(read_gateway_packet, self);
  self->super.handler = do_read_gateway;

  self->connection = connection;
  self->handler = handler;

  self->header = lsh_string_alloc(HEADER_SIZE);
  self->pos = 0;
  self->payload = NULL;

  return &self->super;
}


/* Buffer size when reading from the socket */
#define BUF_SIZE (1<<14)

/* Blocksize when writing */
#define BLOCK_SIZE 2000

static struct ssh_connection *
gateway_make_connection(struct listen_value *lv,
			struct resource *resource,
			struct exception_handler *e)
{
  /* NOTE: lv->peer is usually NULL here. */
  struct ssh_connection *connection
    = make_ssh_connection(0, /* flags */
			  lv->peer, lv->local, "gateway",
			  make_exc_finish_read_handler(lv->fd, e, HANDLER_CONTEXT));

  /* Adopt resources associated with the connection. */
  if (resource)
    remember_resource(connection->resources, resource);
  
  connection_init_io
    (connection,
     io_read_write(lv->fd,
		   make_buffered_read
		     (BUF_SIZE,
		      make_read_gateway
		        (make_packet_debug(&connection->super,
					   ssh_format("%lz received",
						      connection->debug_comment)),
			 connection)),
		   BLOCK_SIZE,
		   make_connection_close_handler(connection)));
  
  connection->write_packet
    = make_packet_debug(make_gateway_pad(&connection->socket->write_buffer->super),
			ssh_format("%lz sent", connection->debug_comment));

  init_connection_service(connection);

  connection->table->open_fallback = &gateway_channel_open_forward;

  connection->dispatch[SSH_MSG_DEBUG] = &connection_forward_handler;
  connection->dispatch[SSH_MSG_IGNORE] = &connection_forward_handler;

  return connection;
}

DEFINE_COMMAND2(gateway_init)
     (struct command_2 *s UNUSED,
      struct lsh_object *a1,
      struct lsh_object *a2,
      struct command_continuation *c,
      struct exception_handler *e)
{
  CAST_SUBTYPE(resource, resource, a1);
  CAST(listen_value, lv, a2);

  COMMAND_RETURN(c, gateway_make_connection(lv, resource, e));
}


/* (gateway_accept main-connection gateway-connection) */
DEFINE_COMMAND2(gateway_accept)
     (struct command_2 *s UNUSED,
      struct lsh_object *a1,
      struct lsh_object *a2,
      struct command_continuation *c,
      struct exception_handler *e)
{
  CAST(ssh_connection, connection, a1);
  CAST(listen_value, lv, a2);

  struct ssh_connection *gateway = gateway_make_connection(lv, NULL, e);
  
  /* Kill gateway connection if the main connection goes down. */
  remember_resource(connection->resources, &lv->fd->super);
  
  gateway->chain = connection;

  COMMAND_RETURN(c, gateway);
}


/* GABA:
   (expr
     (name gateway_setup)
     (params
       (local object local_info))
     (expr
       (lambda (connection)
         (connection_remember connection
	   (listen
	     (lambda (peer)
	       (gateway_accept connection peer))
	       ;; prog1, to delay binding until we're connected.
	       (bind_local (prog1 local connection)) )))))
*/

struct command *
make_gateway_setup(struct local_info *local)
{
  CAST_SUBTYPE(command, res,
	       gateway_setup(local));

  trace("make_gateway_setup\n");

  return res;
}
