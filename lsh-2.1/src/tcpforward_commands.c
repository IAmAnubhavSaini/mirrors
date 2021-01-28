/* tcpforward_commands.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998 Balázs Scheidler, Niels Möller
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

#include "tcpforward.h"

#include "atoms.h"
#include "channel_commands.h"
#include "channel_forward.h"
#include "connection_commands.h"
#include "format.h"
#include "io_commands.h"
#include "ssh.h"
#include "werror.h"
#include "xalloc.h"

/* Forward declarations */

/* FIXME: Should be static */
struct command_2 open_direct_tcpip;
#define OPEN_DIRECT_TCPIP (&open_direct_tcpip.super.super)

/* FIXME: Should be static */
struct command_2 remote_listen_command;
#define REMOTE_LISTEN (&remote_listen_command.super.super)

/* FIXME: Should be static */
struct command_2 open_forwarded_tcpip;
#define OPEN_FORWARDED_TCPIP (&open_forwarded_tcpip.super.super)


struct command tcpip_connect_io_command;
#define TCPIP_CONNECT_IO (&tcpip_connect_io_command.super)

static struct install_info install_direct_tcpip_handler;
#define INSTALL_DIRECT_TCPIP (&install_direct_tcpip_handler.super.super.super)

/* FIXME: Should be static */
struct command make_direct_tcpip_handler;
#define DIRECT_TCPIP_HANDLER (&make_direct_tcpip_handler.super)

static struct install_info install_tcpip_forward_handler;
#define INSTALL_TCPIP_FORWARD (&install_tcpip_forward_handler.super.super.super)

/* FIXME: Should be static? */
struct command make_tcpip_forward_handler;
#define TCPIP_FORWARD_HANDLER (&make_tcpip_forward_handler.super)

#include "tcpforward_commands.c.x"


/* Takes a socket as argument, and returns a tcpip channel. Used by
 * the party receiving a open-tcp request, when a channel to the
 * target has been opened. */

#define TCPIP_WINDOW_SIZE 10000

/* NOTE: make_channel_forward adds the fd to the channel's resource list. */
DEFINE_COMMAND(tcpip_connect_io_command)
     (struct command *ignored UNUSED,
      struct lsh_object *x,
      struct command_continuation *c,
      struct exception_handler *e UNUSED)
{
  CAST(listen_value, lv, x);

  assert(lv);
  assert(lv->fd);
  
  COMMAND_RETURN(c, make_channel_forward(lv->fd, TCPIP_WINDOW_SIZE));
}


/* Requesting the opening of a forwarded tcpip channel. */

/* Used for both forwarded-tcpip and direct-tcpip. Takes a listen
 * value as argument, and returns a channel connected to some tcpip
 * port at the other end. */

/* GABA:
   (class
     (name open_tcpip_command)
     (super channel_open_command)
     (vars
       ; ATOM_FORWARDED_TCPIP or ATOM_DIRECT_TCPIP
       (type . int)

       ;; Appearantly not used for anything.
       ;; (initial_window . uint32_t)

       ; For forwarded-tcpip, port is the port listened to.
       ; For direct-tcpip, port is the port to connect to.
       ; In both cases, it's a port used on the server end.
       (port object address_info)
       (peer object listen_value)))
*/

static struct ssh_channel *
new_tcpip_channel(struct channel_open_command *c,
		  struct ssh_connection *connection,
		  uint32_t local_channel_number,
		  struct lsh_string **request)
{
  CAST(open_tcpip_command, self, c);
  struct ssh_channel *channel;

  /* NOTE: All accepted fd:s must end up in this function, so it
   * should be ok to delay the REMEMBER call until here. It is done
   * by make_channel_forward. */

  debug("tcpforward_commands.c: new_tcpip_channel\n");

  channel = &make_channel_forward(self->peer->fd, TCPIP_WINDOW_SIZE)->super;
  channel->connection = connection;

  *request = format_channel_open(self->type, local_channel_number,
				 channel, 
				 "%S%i%S%i",
				 self->port->ip, self->port->port,
				 self->peer->peer->ip, self->peer->peer->port);
  
  return channel;
}

struct command *
make_open_tcpip_command(int type,
			struct address_info *port,
			struct listen_value *peer)
{
  NEW(open_tcpip_command, self);

  debug("tcpforward_commands.c: make_open_tcpip_command\n");

  self->super.super.call = do_channel_open_command;
  self->super.new_channel = new_tcpip_channel;

  self->type = type;
  
  self->port = port;
  self->peer = peer;
  
  return &self->super.super;
}

DEFINE_COMMAND2(open_forwarded_tcpip)
     (struct command_2 *s UNUSED,
      struct lsh_object *a1,
      struct lsh_object *a2,
      struct command_continuation *c,
      struct exception_handler *e UNUSED)
{
  CAST(address_info, local, a1);
  CAST(listen_value, peer, a2);
  
  COMMAND_RETURN(c,
		 make_open_tcpip_command(ATOM_FORWARDED_TCPIP,
					 local, peer));
}

DEFINE_COMMAND2(open_direct_tcpip)
     (struct command_2 *s UNUSED,
      struct lsh_object *a1,
      struct lsh_object *a2,
      struct command_continuation *c,
      struct exception_handler *e UNUSED)
{
  CAST(address_info, target, a1);
  CAST(listen_value, peer, a2);
  
  COMMAND_RETURN(c,
		 make_open_tcpip_command(ATOM_DIRECT_TCPIP,
					 target, peer));
}


/* Requesting remote forwarding of a port */

/* GABA:
   (class
     (name remote_port_install_continuation)
     (super command_frame)
     (vars
       (port object remote_port)
       (callback object command)))
*/

static void
do_remote_port_install_continuation(struct command_continuation *s,
				    struct lsh_object *x)
{
  CAST(remote_port_install_continuation, self, s);
  CAST(ssh_connection, connection, x);

  assert(connection);

  debug("tcpforward_commands.c: do_remote_port_install_continuation, success.\n");
  self->port->callback = self->callback;

  COMMAND_RETURN(self->super.up, x);
}

static struct command_continuation *
make_remote_port_install_continuation(struct remote_port *port,
				      struct command *callback,
				      struct command_continuation *c)
{
  NEW(remote_port_install_continuation, self);

  debug("tcpforward_commands.c: make_remote_port_install_continuation\n");

  self->super.super.c = do_remote_port_install_continuation;
  self->super.up = c;

  self->port = port;
  self->callback = callback;

  return &self->super.super;
}

/* Listening on a remote port
 *
 * (remote_listen callback port connection)
 *
 * Returns a remote_port or NULL.
 * 
 * callback is invoked with a address_info peer as argument, and
 * should return a channel or NULL.
 */

/* GABA:
   (class
     (name request_tcpip_forward_command)
     (super global_request_command)
     (vars
       ; Invoked when a forwarded_tcpip request is received.
       ; Called with the struct address_info *peer as argument.
       (callback object command)
       (port object address_info))) */

static struct lsh_string *
do_format_request_tcpip_forward(struct global_request_command *s,
				struct ssh_connection *connection,
				struct command_continuation **c)
{
  CAST(request_tcpip_forward_command, self, s);
  struct remote_port *port;
  int want_reply;

  debug("tcpforward_commands.c: do_format_request_tcpip_forward\n");

  if (CONTINUATION_USED_P(*c))
    {
      /* FIXME: Use some exception handler to remove the port from the
       * list if the request fails. */
      port = make_remote_port(self->port, NULL);
      *c = make_remote_port_install_continuation(port, self->callback, *c);
      want_reply = 1;
    }
  else
    {
      port = make_remote_port(self->port, self->callback);
      want_reply = 0;
    }
  
  object_queue_add_tail(&connection->table->remote_ports,
			&port->super.super);
  
  return format_global_request(ATOM_TCPIP_FORWARD, want_reply, "%S%i",
			       self->port->ip, self->port->port);
}


DEFINE_COMMAND2(remote_listen_command)
     (struct command_2 *s UNUSED,
      struct lsh_object *a1,
      struct lsh_object *a2,
      struct command_continuation *c,
      struct exception_handler *e UNUSED)
{
  trace("remote_listen_command\n");

  {
    CAST_SUBTYPE(command, callback, a1);
    CAST(address_info, port, a2);
    
    NEW(request_tcpip_forward_command, self);
    
    self->super.super.call = do_channel_global_command;
    self->super.format_request = do_format_request_tcpip_forward;

    self->callback = callback;
    self->port = port;
    
    COMMAND_RETURN(c, self);
  }
}


/* Cancel a remotely forwarded port.
 * FIXME: Not implemented */



/* GABA:
   (expr
     (name forward_local_port)
     (params
       (local object address_info)
       (target object address_info))
     (expr
       (lambda (connection)
         (connection_remember connection
           (listen
	     (lambda (peer)
	       ;; Remembering is done by open_direct_tcpip
	       ;; and new_tcpip_channel.
	       (start_io
	         (catch_channel_open 
		   (open_direct_tcpip target peer) connection)))
	     ; NOTE: The use of prog1 is needed to delay the bind call
	     ; until the (otherwise ignored) connection argument is
	     ; available.
	     (bind (prog1 local connection)))))))
*/

struct command *
make_forward_local_port(struct address_info *local,
			struct address_info *target)
{
  CAST_SUBTYPE(command, res,
	       forward_local_port(local, target));

  trace("tcpforward_commands.c: forward_local_port\n");

  return res;
}

/* GABA:
   (expr
     (name forward_remote_port)
     (params
       (connect object command)
       (remote object address_info))
     (expr
       (lambda (connection)
         (remote_listen (lambda (peer)
	                  (tcpip_connect_io 
			     ; NOTE: The use of prog1 is needed to
			     ; delay the connect call until the
			     ; (otherwise ignored) peer argument is
			     ; available.  
			     (connect (prog1 connection peer))))
	                remote
			connection))))
*/

struct command *
make_forward_remote_port(struct address_info *remote,
			 struct address_info *target)
{
  CAST_SUBTYPE(command, res,
	       forward_remote_port(make_connect_port(target), remote));

  debug("tcpforward_commands.c: forward_remote_port\n");
  
  return res;
}

/* Takes a callback function and returns a channel_open
 * handler. */
DEFINE_COMMAND(make_direct_tcpip_handler)
     (struct command *s UNUSED,
      struct lsh_object *x,
      struct command_continuation *c,
      struct exception_handler *e UNUSED)
{
  CAST_SUBTYPE(command, callback,  x);

  trace("tcpforward_commands.c: do_make_open_tcp_handler\n");
  
  COMMAND_RETURN(c,
		 &make_channel_open_direct_tcpip(callback)->super);
}

/* FIXME: Merge with install_tcpip_forward_handler */
/* Takes a callback function and returns a global_request handler. */
DEFINE_COMMAND(make_tcpip_forward_handler)
     (struct command *s UNUSED,
      struct lsh_object *x,
      struct command_continuation *c,
      struct exception_handler *e UNUSED)
{
  CAST_SUBTYPE(command, callback,  x);

  debug("tcpforward_commands.c: make_tcpip_forward_handler\n");
  
  COMMAND_RETURN(c,
		 &make_tcpip_forward_request(callback)->super);
}

/* Commands to install handlers */
static struct install_info install_direct_tcpip_handler =
STATIC_INSTALL_OPEN_HANDLER(ATOM_DIRECT_TCPIP);


/* Server side callbacks */

/* Make this non-static? */
/* GABA:
   (expr
     (name direct_tcpip_hook)
     (expr
       (lambda (connection)
         (install_direct_tcpip connection
	   (direct_tcpip_handler (lambda (port)
	     (tcpip_connect_io (connect_connection connection port))))))))
*/

struct command *
make_direct_tcpip_hook(void)
{
  CAST_SUBTYPE(command, res,
	       direct_tcpip_hook());
  
  debug("tcpforward_commands.c: make_direct_tcpip_hook\n");

  return res;
}

static struct install_info install_tcpip_forward_handler =
STATIC_INSTALL_GLOBAL_HANDLER(ATOM_TCPIP_FORWARD);

/* GABA:
   (expr
     (name tcpip_forward_hook)
     (expr
       (lambda (connection)
         ;; Called when the ssh-connection is established
         (install_tcpip_forward connection
	   (tcpip_forward_handler (lambda (port)
	     ;; Called when the client requests remote forwarding.
	     ;; It should return the fd associated with the port.
	     ;; NOTE: The caller, do_tcpip_forward_request, is responsible
	     ;; for handling I/O exceptions, and for remembering the port.
	     (listen (lambda (peer)
  		  	;; Called when someone connects to the
		  	;; forwarded port.
			;; Remembering is done by open_direct_tcpip
			;; and new_tcpip_channel.
			(start_io
		  	  (catch_channel_open 
		  	    (open_forwarded_tcpip port peer) connection)))
		     (bind port) ))))))))
*/

struct command *
make_tcpip_forward_hook(void)
{
  CAST_SUBTYPE(command, res, tcpip_forward_hook());

  debug("tcpforward_commands.c: tcpip_forward_hook\n");
  
  return res;
}
