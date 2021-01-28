/* server_session.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998, 2002 Niels Möller
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>

#include <signal.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>

#include "server_session.h"

#include "channel_commands.h"
#include "environ.h"
#include "format.h"
#include "lsh_string.h"
#include "read_data.h"
#include "reaper.h"
#include "server_pty.h"
#include "server_x11.h"
#include "ssh.h"
#include "tcpforward.h"
#include "translate_signal.h"
#include "tty.h"
#include "werror.h"
#include "xalloc.h"

#include "server_session.c.x"


/* Session */
/* GABA:
   (class
     (name server_session)
     (super ssh_channel)
     (vars
       ; User information
       ;; (user object lsh_user)

       (initial_window . uint32_t)

       ; Resource to kill when the channel is closed. 
       (process object lsh_process)

       ; An allocated but not yet used pty
       (pty object pty_info)

       ; X11 forwarding.
       (x11 object server_x11_info)
       
       ; Value of the TERM environment variable
       (term string)

       ; Value for the SSH_CLIENT environment variable.
       (client string)
       
       ; Child process's stdio 
       (in object lsh_fd)
       (out object lsh_fd)

       ; err may be NULL, if there's no separate stderr channel.
       ; This happens if we use a pty, and the bash workaround is used.
       (err object lsh_fd)))
*/

/* Receive channel data */
static void
do_receive(struct ssh_channel *c,
	   int type, struct lsh_string *data)
{
  CAST(server_session, closure, c);

  switch(type)
    {
    case CHANNEL_DATA:
      A_WRITE(&closure->in->write_buffer->super, data);
      break;
    case CHANNEL_STDERR_DATA:
      werror("Ignoring unexpected stderr data.\n");
      lsh_string_free(data);
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
  CAST(server_session, session, s);

  /* FIXME: Perhaps it's better to just check the read pointers, and
   * not bother with the alive-flags? */
  if (session->out->super.alive)
    {
      assert(session->out->read);

      lsh_oop_register_read_fd(session->out);
    }
  
  if (session->err && session->err->super.alive)
    {
      assert(session->err->read);
  
      lsh_oop_register_read_fd(session->err);
    }
}

static void
do_eof(struct ssh_channel *channel)
{
  CAST(server_session, session, channel);

  trace("server_session.c: do_eof\n");
  
  close_fd_write(session->in);
}

struct ssh_channel *
make_server_session(uint32_t initial_window,
		    struct alist *request_types)
{
  NEW(server_session, self);

  init_channel(&self->super);

  self->initial_window = initial_window;

  /* We don't want to receive any data before we have forked some
   * process to receive it. */
  self->super.rec_window_size = 0;

  /* FIXME: Make maximum packet size configurable. */
  self->super.rec_max_packet = SSH_MAX_PACKET;
  self->super.request_types = request_types;

  /* Note: We don't need a close handler; the channels resource list
   * is taken care of automatically. */
  
  self->process = NULL;

  self->pty = NULL;
  self->term = NULL;
  self->client = NULL;
  
  self->in = NULL;
  self->out = NULL;
  self->err = NULL;
  
  return &self->super;
}


/* GABA:
   (class
     (name open_session)
     (super channel_open)
     (vars
       (session_requests object alist)))
*/

#define WINDOW_SIZE 10000

static void
do_open_session(struct channel_open *s,
		struct ssh_connection *connection UNUSED,
		struct channel_open_info *info UNUSED,
		struct simple_buffer *args,
		struct command_continuation *c,
		struct exception_handler *e)
{
  CAST(open_session, self, s);

  debug("server.c: do_open_session\n");

  assert(connection->user);
  
  if (parse_eod(args))
    {
      COMMAND_RETURN(c,
		     make_server_session(WINDOW_SIZE, self->session_requests));
    }
  else
    {
      PROTOCOL_ERROR(e, "trailing garbage in open message");
    }
}

struct channel_open *
make_open_session(struct alist *session_requests)
{
  NEW(open_session, closure);

  closure->super.handler = do_open_session;
  closure->session_requests = session_requests;
  
  return &closure->super;
}


struct lsh_string *
format_exit_signal(struct ssh_channel *channel,
		   int core, int signal)
{
  struct lsh_string *msg
    = ssh_format("%lz.\n", STRSIGNAL(signal));
  
  return format_channel_request(ATOM_EXIT_SIGNAL,
				channel,
				0,
				"%a%c%fS%z",
				signal_local_to_network(signal),
				core,
				msg, "");
}

struct lsh_string *
format_exit(struct ssh_channel *channel, int value)
{
  return format_channel_request(ATOM_EXIT_STATUS,
				channel,
				0,
				"%i", value);
}

/* GABA:
   (class
     (name exit_shell)
     (super exit_callback)
     (vars
       (session object server_session)))
*/

static void
do_exit_shell(struct exit_callback *c, int signaled,
	      int core, int value)
{
  CAST(exit_shell, closure, c);
  struct server_session *session = closure->session;
  struct ssh_channel *channel = &session->super;
  
  trace("server_session.c: do_exit_shell\n");
  
  /* NOTE: We don't close the child's stdio here. */

  if (!(channel->flags & CHANNEL_SENT_CLOSE))
    {
      verbose("server_session.c: Sending %a message on channel %i\n",
	      signaled ? ATOM_EXIT_SIGNAL : ATOM_EXIT_STATUS,
	      channel->channel_number);
      
      connection_send(channel->connection,
		      (signaled
		       ? format_exit_signal(channel, core, value)
		       : format_exit(channel, value)) );

      /* We want to close the channel as soon as all stdout and stderr
       * data has been sent. In particular, we don't wait for EOF from
       * the client, most clients never sends that. */
      
      channel->flags |= (CHANNEL_NO_WAIT_FOR_EOF | CHANNEL_CLOSE_AT_EOF);
      
      if (channel->flags & CHANNEL_SENT_EOF)
	{
	  /* We have sent EOF already, so initiate close */
	  channel_close(channel);
	}
    }
}

static struct exit_callback *
make_exit_shell(struct server_session *session)
{
  NEW(exit_shell, self);

  self->super.exit = do_exit_shell;
  self->session = session;

  return &self->super;
}


static int
make_pipes(int *in, int *out, int *err)
{
  int saved_errno;
  
  if (lsh_make_pipe(in))
    {
      if (lsh_make_pipe(out))
	{
	  if (lsh_make_pipe(err))
	    {
              return 1;
            }
	  saved_errno = errno;
          close(out[0]);
          close(out[1]);
        }
      else
	saved_errno = errno;
      close(in[0]);
      close(in[1]);
    }
  else
    saved_errno = errno;
  
  errno = saved_errno;
  return 0;
}

#define BASH_WORKAROUND 1

#if WITH_PTY_SUPPORT

/* Sets certain fd:s to -1, which means that the slave tty should be
 * used (for the child), or that the stdout fd should be duplicated
 * (for the parent). */
static int
make_pty(struct pty_info *pty, int *in, int *out, int *err)
{
  debug("make_pty... ");

  assert(pty);
  
  debug("exists: \n"
        "  alive = %i\n"
        "  master = %i\n"
        "... ",
        pty->super.alive, pty->master);
  debug("\n");
  
  if (pty) 
    {
      assert(pty->super.alive);
      
      debug("make_pty: Using allocated pty.\n");

      /* Ownership of the master fd is passed on to some file
       * object. We need an fd for window_change_request, but we have
       * to use our regular fd:s to the master side, or we're
       * disrupt EOF handling on either side. */

      pty->super.alive = 0;
      
      /* FIXME: It seems unnecessary to dup all the fd:s here. We
       * could use a single lsh_fd object for the master side of the
       * pty. */

      /* -1 means opening deferred to the child */
      in[0] = -1;
      in[1] = pty->master;
      
      if ((out[0] = dup(pty->master)) < 0)
        {
          werror("make_pty: duping master pty to stdout failed %e\n", errno);

          return 0;
        }

      out[1] = -1;

      /* pty_info no longer owns the pty fd */
      pty->master = -1;

#if BASH_WORKAROUND
      /* Don't use a separate stderr channel; just dup the
       * stdout pty to stderr. */
            
      err[0] = -1;
      err[1] = -1;
      
#else /* !BASH_WORKAROUND */
      if (!lsh_make_pipe(err))
        {
          close(in[1]);
          close(out[0]);
	  
          return 0;
        }
#endif /* !BASH_WORKAROUND */
      return 1;
    }
  return 0;
}

#else /* !WITH_PTY_SUPPORT */
static int make_pty(struct pty_info *pty UNUSED,
		    int *in UNUSED, int *out UNUSED, int *err UNUSED)
{ return 0; }
#endif /* !WITH_PTY_SUPPORT */

static int
spawn_process(struct server_session *session,
	      struct lsh_user *user,
	      /* All information but the fd:s should be filled in
	       * already */
	      struct spawn_info *info)
{
  struct lsh_callback *read_close_callback;
  
  assert(!session->process);

  if (session->pty && !make_pty(session->pty,
				info->in, info->out, info->err))
    {
      KILL_RESOURCE(&session->pty->super);
      KILL(session->pty);
      session->pty = NULL;
    }

  if (!session->pty && !make_pipes(info->in, info->out, info->err))
    return 0;

  session->process = USER_SPAWN(user, info, make_exit_shell(session));

  if (!session->process)
    return 0;
  
  /* Close callback for stderr and stdout */
  read_close_callback
    = make_channel_read_close_callback(&session->super);
  
  session->in
    = io_write(make_lsh_fd
	       (info->in[1], session->pty ? IO_PTY : IO_NORMAL,
		"child stdin",
		make_channel_io_exception_handler(&session->super,
						  "Child stdin: ", 0,
						  &default_exception_handler,
						  HANDLER_CONTEXT)),
	       SSH_MAX_PACKET, NULL);

  /* Flow control */
  session->in->write_buffer->report = &session->super.super;
  
  /* FIXME: Should we really use the same exception handler,
   * which will close the channel on read errors, or is it
   * better to just send EOF on read errors? */
  session->out
    = io_read(make_lsh_fd
	      (info->out[0], IO_NORMAL, "child stdout",
	       make_channel_io_exception_handler(&session->super,
						 "Child stdout: ", 1,
						 &default_exception_handler,
						 HANDLER_CONTEXT)),
	      make_channel_read_data(&session->super),
	      read_close_callback);
  session->err 
    = ( (info->err[0] != -1)
	? io_read(make_lsh_fd
		  (info->err[0], IO_NORMAL, "child stderr",
		   make_channel_io_exception_handler(&session->super,
						     "Child stderr: ", 0,
						     &default_exception_handler,
						     HANDLER_CONTEXT)),
		  make_channel_read_stderr(&session->super),
		  read_close_callback)
	: NULL);

  session->super.receive = do_receive;
  session->super.send_adjust = do_send_adjust;
  session->super.eof = do_eof;
	  
  /* Make sure that the process and it's stdio is
   * cleaned up if the channel or connection dies. */
  remember_resource(session->super.resources, &session->process->super);

  /* FIXME: How to do this properly if in and out may use the
   * same fd? */
  remember_resource(session->super.resources, &session->in->super);
  remember_resource(session->super.resources, &session->out->super);
  if (session->err)
    remember_resource(session->super.resources, &session->err->super);

  /* Don't close channel immediately at EOF, as we want to
   * get a chance to send exit-status or exit-signal. */
  session->super.flags &= ~CHANNEL_CLOSE_AT_EOF;

  channel_start_receive(&session->super, session->initial_window);
  
  return 1;
}

static void
init_spawn_info(struct spawn_info *info, struct server_session *session,
		unsigned argc, const char **argv,
		unsigned env_length, struct env_value *env)
{
  unsigned i = 0;
  
  memset(info, 0, sizeof(*info));
  info->peer = session->super.connection->peer;
  info->pty = session->pty;

  info->argc = argc;
  info->argv = argv;
  
  assert(env_length >= 5);

  /* FIXME: Set SSH_ORIGINAL_COMMAND */
  if (session->term)
    {
      env[i].name = ENV_TERM;
      env[i].value = session->term;
      i++;
    }

  if (info->pty && info->pty->tty_name)
    {
      env[i].name = ENV_SSH_TTY;
      env[i].value = info->pty->tty_name;
      i++;
    }

  if (info->peer)
    {
      /* Save string in the session object, so that it can be garbage
       * collected properly. */
      assert(!session->client);
      
      if (session->super.connection->local)
	session->client = ssh_format("%lS %di %di", 
				      info->peer->ip,
				      info->peer->port,
				      session->super.connection->local->port);
      else
	session->client = ssh_format("%lS %di UNKNOWN", 
				     info->peer->ip,
				     info->peer->port
				     );
      env[i].name = ENV_SSH_CLIENT;
      env[i].value = session->client;

      i++;
    }

#if WITH_X11_FORWARD
  if (session->x11)
    {
      env[i].name = ENV_DISPLAY;
      env[i].value = session->x11->display;
      i++;

      env[i].name = ENV_XAUTHORITY;
      env[i].value = session->x11->xauthority;
      i++;
    }
#endif /* WITH_X11_FORWARD */
  assert(i <= env_length);
  info->env_length = i;
  info->env = env;
}

DEFINE_CHANNEL_REQUEST(shell_request_handler)
     (struct channel_request *s UNUSED,
      struct ssh_channel *channel,
      struct channel_request_info *info UNUSED,
      struct simple_buffer *args,
      struct command_continuation *c,
      struct exception_handler *e)
{
  CAST(server_session, session, channel);
  struct spawn_info spawn;
  struct env_value env[5];
  
  static const struct exception shell_request_failed =
    STATIC_EXCEPTION(EXC_CHANNEL_REQUEST, "Shell request failed");

  if (!parse_eod(args))
    {
      PROTOCOL_ERROR(e, "Invalid shell CHANNEL_REQUEST message.");
      return;
    }
    
  if (session->process)
    /* Already spawned a shell or command */
    goto fail;

  init_spawn_info(&spawn, session, 0, NULL, 5, env);
  spawn.login = 1;
    
  if (spawn_process(session, channel->connection->user, &spawn))
    COMMAND_RETURN(c, channel);
  else
    {
    fail:
      EXCEPTION_RAISE(e, &shell_request_failed);
    }
}

DEFINE_CHANNEL_REQUEST(exec_request_handler)
     (struct channel_request *s UNUSED,
      struct ssh_channel *channel,
      struct channel_request_info *info UNUSED,
      struct simple_buffer *args,
      struct command_continuation *c,
      struct exception_handler *e)
{
  CAST(server_session, session, channel);

  static const struct exception exec_request_failed =
    STATIC_EXCEPTION(EXC_CHANNEL_REQUEST, "Exec request failed");
  
  uint32_t command_len;
  const uint8_t *command;

  if (!(parse_string(args, &command_len, &command)
	&& parse_eod(args)))
    {
      PROTOCOL_ERROR(e, "Invalid exec CHANNEL_REQUEST message.");
      return;
    }
    
  if (/* Already spawned a shell or command */
      session->process
      /* Command can't contain NUL characters. */
      || memchr(command, '\0', command_len))
    
    EXCEPTION_RAISE(e, &exec_request_failed);
  else
    {
      struct spawn_info spawn;
      const char *args[2];
      struct env_value env[5];

      struct lsh_string *s = ssh_format("%ls", command_len, command);
      
      init_spawn_info(&spawn, session, 2, args, 5, env);
      spawn.login = 0;
      
      args[0] = "-c";
      args[1] = lsh_get_cstring(s);

      if (spawn_process(session, channel->connection->user, &spawn))
	COMMAND_RETURN(c, channel);
      else
	EXCEPTION_RAISE(e, &exec_request_failed);

      lsh_string_free(s);
    }
}

/* For simplicity, represent a subsystem simply as a name of the
 * executable. */

/* GABA:
   (class
     (name subsystem_request)
     (super channel_request)
     (vars
       ;(subsystems object alist)
       ; A list { name, program, name, program, NULL }
       (subsystems . "const char **")))
*/

/* ;; GABA:
   (class
     (name sybsystem_info)
     (vars
       (name "const char *")))
*/

static const char *
lookup_subsystem(struct subsystem_request *self,
		 uint32_t length, const uint8_t *name)
{
  unsigned i;
  if (memchr(name, 0, length))
    return NULL;

  for (i = 0; self->subsystems[i]; i+=2)
    {
      assert(self->subsystems[i+1]);
      if ((length == strlen(self->subsystems[i]))
	  && !memcmp(name, self->subsystems[i], length))
	return self->subsystems[i + 1];
    }
  return NULL;
}

static void
do_spawn_subsystem(struct channel_request *s,
		   struct ssh_channel *channel,
		   struct channel_request_info *info UNUSED,
		   struct simple_buffer *args,
		   struct command_continuation *c,
		   struct exception_handler *e)
{
  CAST(subsystem_request, self, s);
  CAST(server_session, session, channel);

  static struct exception subsystem_request_failed =
    STATIC_EXCEPTION(EXC_CHANNEL_REQUEST, "Subsystem request failed");

  const uint8_t *name;
  uint32_t name_length;

  const char *program;
      
  if (! (parse_string(args, &name_length, &name) && parse_eod(args)))
    {
      PROTOCOL_ERROR(e, "Invalid subsystem CHANNEL_REQUEST message.");
      return;
    }
  
  program = lookup_subsystem(self, name_length, name);
  
  if (!session->process && program)
    {
      struct spawn_info spawn;
      const char *args[2];
      struct env_value env[5];

      /* Don't use any pty */
      if (session->pty)
	{
	  KILL_RESOURCE(&session->pty->super);
	  session->pty = NULL;
	}

      args[0] = "-c"; args[1] = program;
      
      init_spawn_info(&spawn, session, 2, args, 5, env);
      spawn.login = 0;

      if (spawn_process(session, channel->connection->user, &spawn))
	{
	  COMMAND_RETURN(c, channel);
	  return;
	}  
    }
  EXCEPTION_RAISE(e, &subsystem_request_failed);
}

struct channel_request *
make_subsystem_handler(const char **subsystems)
{
  NEW(subsystem_request, self);

  self->super.handler = do_spawn_subsystem;
  self->subsystems = subsystems;
  
  return &self->super;
}


#if WITH_PTY_SUPPORT

/* pty_handler */
static void
do_alloc_pty(struct channel_request *c UNUSED,
	     struct ssh_channel *channel,
	     struct channel_request_info *info UNUSED,
	     struct simple_buffer *args,
	     struct command_continuation *s,
	     struct exception_handler *e)
{
  struct lsh_string *term = NULL;

  static struct exception pty_request_failed =
    STATIC_EXCEPTION(EXC_CHANNEL_REQUEST, "pty request failed");

  struct pty_info *pty = make_pty_info();
  
  CAST(server_session, session, channel);

  verbose("Client requesting a tty...\n");

  if ((term = parse_string_copy(args))
      && parse_uint32(args, &pty->dims.char_width)
      && parse_uint32(args, &pty->dims.char_height)
      && parse_uint32(args, &pty->dims.pixel_width)
      && parse_uint32(args, &pty->dims.pixel_height)
      && (pty->mode = parse_string_copy(args))
      && parse_eod(args))
    {
      /* The client may only request one tty, and only before
       * starting a process. */
      if (session->pty || session->process
	  || !pty_open_master(pty, channel->connection->user->uid))
	{
	  verbose("Pty allocation failed.\n");
	  EXCEPTION_RAISE(e, &pty_request_failed);
	}
      else
	{
	  /* FIXME: Perhaps we can set the window dimensions directly
	   * on the master pty? */
	  session->term = term;
	  session->pty = pty;
	  remember_resource(channel->resources, &pty->super);

	  verbose(" ... granted.\n");
	  COMMAND_RETURN(s, channel);

	  /* Success */
	  return;
	}

    }
  else
    {
      werror("Invalid pty request.\n");
      PROTOCOL_ERROR(e, "Invalid pty request.");
    }
  /* Cleanup for failure cases. */
  lsh_string_free(term);
  KILL_RESOURCE(&pty->super);
  KILL(pty);
}

struct channel_request
pty_request_handler =
{ STATIC_HEADER, do_alloc_pty };

static void
do_window_change_request(struct channel_request *c UNUSED,
			 struct ssh_channel *channel,
			 struct channel_request_info *info UNUSED,
			 struct simple_buffer *args,
			 struct command_continuation *s,
			 struct exception_handler *e)
{
  struct terminal_dimensions dims;
  CAST(server_session, session, channel);

  verbose("Receiving window-change request...\n");

  if (parse_uint32(args, &dims.char_width)
      && parse_uint32(args, &dims.char_height)
      && parse_uint32(args, &dims.pixel_width)
      && parse_uint32(args, &dims.pixel_height)
      && parse_eod(args))
    {
      static const struct exception winch_request_failed =
	STATIC_EXCEPTION(EXC_CHANNEL_REQUEST, "window-change request failed: No pty");

      if (session->pty && session->in && session->in->super.alive
          && tty_setwinsize(session->in->fd, &dims))
        /* Success. Rely on the terminal driver sending SIGWINCH */
        COMMAND_RETURN(s, channel);
      else
        EXCEPTION_RAISE(e, &winch_request_failed);
    }
  else
    PROTOCOL_ERROR(channel->connection->e, "Invalid window-change request.");
}

struct channel_request
window_change_request_handler =
{ STATIC_HEADER, do_window_change_request };

#endif /* WITH_PTY_SUPPORT */

#if WITH_X11_FORWARD

/* FIXME: We must delay the handling of any shell request until
 * we have responded to the x11-req message. */
static void
do_x11_req(struct channel_request *s UNUSED,
	   struct ssh_channel *channel,
	   struct channel_request_info *info UNUSED,
	   struct simple_buffer *args,
	   struct command_continuation *c,
	   struct exception_handler *e)
{
  static struct exception x11_request_failed =
    STATIC_EXCEPTION(EXC_CHANNEL_REQUEST, "x11-req failed");

  const uint8_t *protocol;
  uint32_t protocol_length;
  const uint8_t *cookie;
  uint32_t cookie_length;
  uint32_t screen;
  unsigned single;
  
  CAST(server_session, session, channel);

  verbose("Client requesting x11 forwarding...\n");

  if (parse_uint8(args, &single)
      && parse_string(args, &protocol_length, &protocol)
      && parse_string(args, &cookie_length, &cookie)
      && parse_uint32(args, &screen))
    {
      /* The client may only request one x11-forwarding, and only
       * before starting a process. */
      if (session->x11 || session->process
	  || !(session->x11 = server_x11_setup(channel,
					       channel->connection->user,
					       single,
					       protocol_length, protocol,
					       cookie_length, cookie,
					       screen, c, e)))
	{
	  verbose("X11 request failed.\n");
	  EXCEPTION_RAISE(e, &x11_request_failed);
	}
      else
	{	  
	  return;
	}
    }
  else
    {
      werror("Invalid x11 request.\n");
      PROTOCOL_ERROR(e, "Invalid x11 request.");
    }
}

struct channel_request
x11_req_handler =
{ STATIC_HEADER, do_x11_req };

#endif /* WITH_X11_FORWARD */
