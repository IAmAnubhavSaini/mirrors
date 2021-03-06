/* client_session.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998, 1999, 2000 Niels M�ller
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

#include "client.h"

#include "channel_commands.h"
#include "client.h"
#include "io.h"
#include "read_data.h"
#include "ssh.h"
#include "werror.h"
#include "xalloc.h"

#include "client_session.c.x"

/* Initiate and manage a session */
/* GABA:
   (class
     (name client_session_channel)
     (super ssh_channel)
     (vars
       ; To access stdio
       (in object lsh_fd)
       (out object lsh_fd)
       (err object lsh_fd)

       ; Escape char handling
       (escape object escape_info)
       ; Where to save the exit code.
       (exit_status . "int *")))
*/

/* Callback used when the server sends us eof */

static void
do_client_session_eof(struct ssh_channel *c)
{
  CAST(client_session_channel, session, c);

  close_fd_nicely(session->out);
  close_fd_nicely(session->err);
}  

static void
do_client_session_close(struct ssh_channel *c)
{
  static const struct exception finish_exception
    = STATIC_EXCEPTION(EXC_FINISH_PENDING, "Session closed.");

  EXCEPTION_RAISE(c->e, &finish_exception);
}


/* Receive channel data */
static void
do_receive(struct ssh_channel *c,
	   int type, struct lsh_string *data)
{
  CAST(client_session_channel, closure, c);
  
  switch(type)
    {
    case CHANNEL_DATA:
      A_WRITE(&closure->out->write_buffer->super, data);
      break;
    case CHANNEL_STDERR_DATA:
      A_WRITE(&closure->err->write_buffer->super, data);
      break;
    default:
      fatal("Internal error!\n");
    }
}

/* We may send more data */
static void
do_send_adjust(struct ssh_channel *s,
	       uint32_t i UNUSED)
{
  CAST(client_session_channel, self, s);

  assert(self->in->read);

  lsh_oop_register_read_fd(self->in);
}

/* Escape char handling */

static struct io_callback *
client_read_stdin(struct client_session_channel *session)
{
  struct abstract_write *write = make_channel_write(&session->super);

  if (session->escape)
    write = make_handle_escape(session->escape, write);
  
  return make_read_data(&session->super, write);
}

/* We have a remote shell */
static void
do_client_io(struct command *s UNUSED,
	     struct lsh_object *x,
	     struct command_continuation *c,
	     struct exception_handler *e UNUSED)

{
  CAST(client_session_channel, session, x);
  struct ssh_channel *channel = &session->super;
  assert(x);

  /* Set up write fd:s. */
  
  channel->receive = do_receive;

  /* FIXME: It seems a little kludgy to modify exception handlers
   * here; it would be better to create the fd-objects at a point
   * where the right exception handlers can be installed from the
   * start. */
  session->out->e
    = make_channel_io_exception_handler(channel,
					"stdout: ", 0,
					session->out->e,
					HANDLER_CONTEXT);

  session->err->e
    = make_channel_io_exception_handler(channel,
					"stderr: ", 0,
					session->err->e,
					HANDLER_CONTEXT);

  /* Set up the fd we read from. */
  channel->send_adjust = do_send_adjust;

  /* Setup escape char handler, if appropriate. */
  session->in->read = client_read_stdin(session);

  /* FIXME: Perhaps there is some way to arrange that channel.c calls
   * the CHANNEL_SEND_ADJUST method instead? */
  if (session->super.send_window_size)
    lsh_oop_register_read_fd(session->in);

  /* FIXME: We should also arrange so that the tty is reset before we
   * close it. */
  session->in->close_callback
    = make_channel_read_close_callback(channel);

  /* Make sure stdio is closed properly if the channel or connection dies */
  remember_resource(channel->resources, &session->in->super);
  remember_resource(channel->resources, &session->out->super);
  remember_resource(channel->resources, &session->err->super);
  
  ALIST_SET(channel->request_types, ATOM_EXIT_STATUS,
	    &make_handle_exit_status(session->exit_status)->super);
  ALIST_SET(channel->request_types, ATOM_EXIT_SIGNAL,
	    &make_handle_exit_signal(session->exit_status)->super);

  channel->eof = do_client_session_eof;
      
  COMMAND_RETURN(c, channel);
}

struct command client_io =
{ STATIC_HEADER, do_client_io };


struct ssh_channel *
make_client_session_channel(struct lsh_fd *in,
			    struct lsh_fd *out,
			    struct lsh_fd *err,
			    struct escape_info *escape,
			    uint32_t initial_window,
			    int *exit_status)
{
  NEW(client_session_channel, self);

  trace("make_client_session_channel\n");
  init_channel(&self->super);

  /* Makes sure the pending_close bit is set whenever this session
   * dies, no matter when or how. */
  self->super.close = do_client_session_close;

  /* We could miss the server's exit-status or exit-signal message if
   * we close the channel directly at EOF. So don't do that.
   *
   * FIXME: Perhaps we need to set this bit again in do_exit_status
   * and do_exit_signal. */
  self->super.flags &= ~CHANNEL_CLOSE_AT_EOF;
  
  /* FIXME: We make rec_window_size non-zero here, but we don't setup
   * the receive pointer until later, in do_client_io. That's bad. Do
   * something similar to server_session.c: Add an inital_window
   * attribute, and call channel_start_receive from client_io. */
  self->super.rec_window_size = initial_window;

  /* FIXME: Make maximum packet size configurable */
  self->super.rec_max_packet = SSH_MAX_PACKET;

  self->super.request_types = make_alist(0, -1);

  /* self->expect_close = 0; */
  self->in = in;
  self->out = out;
  self->err = err;
  self->escape = escape;

#if 0
  /* Implement send break */
  if (self->escape)
    self->escape->dispatch['b'] = make_send_break(self->super);
#endif
  remember_resource(self->super.resources, &in->super);
  remember_resource(self->super.resources, &out->super);
  remember_resource(self->super.resources, &err->super);
  
  /* Flow control */
  out->write_buffer->report = &self->super.super;
  err->write_buffer->report = &self->super.super;
  
  self->exit_status = exit_status;
  
  return &self->super;
}
