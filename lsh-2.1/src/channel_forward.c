/* channel_forward.h
 *
 * General channel type for forwarding data to an fd
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998, 2001 Balázs Scheidler, Niels Möller
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
#include <errno.h>
#include <string.h>

#include "channel_forward.h"

#include "io.h"
#include "ssh.h"
#include "werror.h"
#include "xalloc.h"

#define GABA_DEFINE
#include "channel_forward.h.x"
#undef GABA_DEFINE

/* NOTE: Adds the socket to the channel's resource list */
void
init_channel_forward(struct channel_forward *self,
		     struct lsh_fd *socket, uint32_t initial_window)
{
  assert(socket);
  
  init_channel(&self->super);

  /* The rest of the callbacks are not set up until
   * channel_forward_start_io. */

  /* NOTE: We don't need a close handler, as the channel's resource
   * list is taken care of automatically. */
  
  self->super.rec_window_size = initial_window;

  /* FIXME: Make maximum packet size configurable. */
  self->super.rec_max_packet = SSH_MAX_PACKET;
  
  self->socket = socket;

  remember_resource(self->super.resources, &socket->super);
}

struct channel_forward *
make_channel_forward(struct lsh_fd *socket, uint32_t initial_window)
{
  NEW(channel_forward, self);
  init_channel_forward(self, socket, initial_window);
  
  return self;
}

static void
do_channel_forward_receive(struct ssh_channel *c,
			   int type, struct lsh_string *data)
{
  CAST_SUBTYPE(channel_forward, closure, c);
  
  switch (type)
    {
    case CHANNEL_DATA:
      A_WRITE(&closure->socket->write_buffer->super, data);
      break;
    case CHANNEL_STDERR_DATA:
      werror("Ignoring unexpected stderr data.\n");
      lsh_string_free(data);
      break;
    default:
      fatal("Internal error. do_channel_forward_receive");
    }
}

static void
do_channel_forward_send_adjust(struct ssh_channel *s,
			       uint32_t i UNUSED)
{
  CAST_SUBTYPE(channel_forward, self, s);
  
  lsh_oop_register_read_fd(self->socket);
}

static void
do_channel_forward_eof(struct ssh_channel *s)
{
  CAST_SUBTYPE(channel_forward, self, s);

  /* We won't write any more. io.c should make sure that shutdown is called
   * once the write_buffer is empty. */
  close_fd_write(self->socket);
}


/* NOTE: Because this function is called by
 * do_open_forwarded_tcpip_continuation, the same restrictions apply.
 * I.e we can not assume that the channel is completely initialized
 * (channel_open_continuation has not yet done its work), and we can't
 * send any packets. */
void
channel_forward_start_io(struct channel_forward *channel)
{
  channel->super.receive = do_channel_forward_receive;
  channel->super.send_adjust = do_channel_forward_send_adjust;
  channel->super.eof = do_channel_forward_eof;
  
  /* Install callbacks on the local socket.
   * NOTE: We don't install any channel_io_exception_handler. */
  io_read_write(channel->socket,
		make_channel_read_data(&channel->super),
		/* FIXME: Make this configurable */
		SSH_MAX_PACKET * 10, /* self->block_size, */
		make_channel_read_close_callback(&channel->super));
  
  /* Flow control */
  channel->socket->write_buffer->report = &channel->super.super;
}

/* Like above, but doesn't install a new write buffer. Used by the socks server. */
void
channel_forward_start_io_read(struct channel_forward *channel)
{
  channel->super.receive = do_channel_forward_receive;
  channel->super.send_adjust = do_channel_forward_send_adjust;
  channel->super.eof = do_channel_forward_eof;
  
  /* Install callbacks on the local socket. Leave write callbacks alone */
  io_read(channel->socket,
	  make_channel_read_data(&channel->super),
	  make_channel_read_close_callback(&channel->super));
  
  /* Flow control */
  channel->socket->write_buffer->report = &channel->super.super;
}

/* Used by the party requesting tcp forwarding, i.e. when a socket is
 * already open, and we have asked the other end to forward it. Takes
 * a channel as argument, and connects it to the socket. Returns the
 * channel. */

DEFINE_COMMAND(start_io_command)
     (struct command *s UNUSED,
      struct lsh_object *x,
      struct command_continuation *c,
      struct exception_handler *e UNUSED)
{
  CAST_SUBTYPE(channel_forward, channel, x);

  assert(channel);
  
  channel_forward_start_io(channel);

  COMMAND_RETURN(c, channel);  
}

/* FIXME: Arrange so that the forwarded socket is closed if
   EXC_CHANNEL_OPEN is caught. And write some docs. */
static const struct report_exception_info forward_open_report =
STATIC_REPORT_EXCEPTION_INFO(EXC_ALL, EXC_CHANNEL_OPEN,
			     "Forwarding failed, could not open channel");

struct catch_report_collect catch_channel_open
= STATIC_CATCH_REPORT(&forward_open_report);
