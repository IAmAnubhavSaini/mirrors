/* client.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998, 1999, 2000 Niels Möller
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

#include <fcntl.h>
#include <signal.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include "client.h"

#include "abstract_io.h"
#include "channel.h"
#include "channel_commands.h"
#include "connection.h"
#include "environ.h"
#include "format.h"
#include "interact.h"
#include "io.h"
#include "lsh_string.h"
#include "parse.h"
#include "ssh.h"
#include "suspend.h"
#include "tcpforward.h"
#include "translate_signal.h"
#include "werror.h"
#include "xalloc.h"
#include "io.h"

#include "lsh_argp.h"

#define GABA_DEFINE
#include "client.h.x"
#undef GABA_DEFINE

#include "client.c.x"

#define DEFAULT_ESCAPE_CHAR '~'
#define DEFAULT_SOCKS_PORT 1080

static struct lsh_string *
format_service_request(int name)
{
  return ssh_format("%c%a", SSH_MSG_SERVICE_REQUEST, name);
}

/* Start a service that the server has accepted (for instance
 * ssh-userauth). */
/* GABA:
   (class
     (name accept_service_handler)
     (super packet_handler)
     (vars
       (service . int)
       (c object command_continuation)))
*/

static void
do_accept_service(struct packet_handler *c,
		  struct ssh_connection *connection,
		  struct lsh_string *packet)
{
  CAST(accept_service_handler, closure, c);

  struct simple_buffer buffer;
  unsigned msg_number;
  int name;

  simple_buffer_init(&buffer, STRING_LD(packet));
  
  if (parse_uint8(&buffer, &msg_number)
      && (msg_number == SSH_MSG_SERVICE_ACCEPT)
      && parse_atom(&buffer, &name)
      && (name == closure->service)
      && parse_eod(&buffer))
    {
      connection->dispatch[SSH_MSG_SERVICE_ACCEPT] = &connection_fail_handler;
      
      COMMAND_RETURN(closure->c, connection);
    }
  else
    PROTOCOL_ERROR(connection->e, "Invalid SSH_MSG_SERVICE_ACCEPT message");
}

struct packet_handler *
make_accept_service_handler(uint32_t service,
			    struct command_continuation *c)
{
  NEW(accept_service_handler, closure);

  closure->super.handler = do_accept_service;
  closure->service = service;
  closure->c = c;
  
  return &closure->super;
}

void
do_request_service(struct command *s,
		   struct lsh_object *x,
		   struct command_continuation *c,
		   struct exception_handler *e UNUSED)
{
  CAST(request_service, self, s);
  CAST(ssh_connection, connection, x);

  /* NOTE: Uses the connection's exception handler, not the one passed
   * in. */
  connection->dispatch[SSH_MSG_SERVICE_ACCEPT]
    = make_accept_service_handler(self->service, c);
  
  connection_send(connection,
		  format_service_request(self->service));
}

struct command *
make_request_service(int service)
{
  NEW(request_service, closure);

  closure->super.call = do_request_service;
  closure->service = service;

  return &closure->super;
}


/* GABA:
   (class
     (name detach_callback)
     (super lsh_callback)
     (vars 
       (channel_flag . int)
       (fd_flag . int)
       (exit_status . "int *")))
*/

/* GABA:
   (class
     (name detach_resource)
     (super resource)
     (vars
       (c object detach_callback)))
*/

static void 
do_detach_res_kill(struct resource *r)
{
  CAST(detach_resource,self,r);

  trace("client.c:do_detach_res\n");
  self->c->channel_flag = 1;

  if (self->c->channel_flag && self->c->fd_flag)
    /* If the fd_flag is set, the callback should be changed */
    io_callout(&self->c->super, 0);
}

static struct resource*
make_detach_resource(struct lsh_callback *c)
{
   NEW(detach_resource, self);
   CAST(detach_callback, cb, c);

   trace("client.c:make_detach_resource\n");
   init_resource(&self->super, do_detach_res_kill);

   self->c = cb;

   return &self->super;
}


static void 
do_detach_cb(struct lsh_callback *c)
{
  CAST(detach_callback,self,c);

  trace("client.c: do_detach_cb\n");
  
  if (!self->fd_flag) /* First time around? */
    {
      self->fd_flag = 1; /* Note */
      
      if (self->channel_flag && self->fd_flag)
	/* If the fd is closed already, ask to be called from the main loop */ 
	io_callout(c, 0);
    }
  else
    {
      int pid = fork();

      /* Ignore any errors, what can we do? */
      
      switch(pid)
	{
	case -1: /* Fork failed, this we can handle by doing nothing */
	  werror("Fork failed, not detaching.\n");
	  break;
	  
	case 0:
	  /* Detach */	  
	  close(STDIN_FILENO); 
	  close(STDOUT_FILENO); 
	  close(STDERR_FILENO); 
	  
	  /* Make sure they aren't used by any file lsh opens */
	  
	  open("/dev/null", O_RDONLY);
	  open("/dev/null", O_RDONLY);
	  open("/dev/null", O_RDONLY);
	  break;
	  
	default:
	  exit(*self->exit_status);
	}
    }
}

static struct lsh_callback* 
make_detach_callback(int *exit_status)
{
   NEW(detach_callback, self);

   self->super.f = do_detach_cb;
   self->exit_status = exit_status;
   self->fd_flag = 0;
   self->channel_flag = 0;

   return &self->super;
}

/* GABA:
   (class
     (name exit_handler)
     (super channel_request)
     (vars
       (exit_status . "int *")))
*/

static void
do_exit_status(struct channel_request *c,
	       struct ssh_channel *channel,
	       struct channel_request_info *info,
	       struct simple_buffer *args,
	       struct command_continuation *s,
	       struct exception_handler *e)
{
  CAST(exit_handler, closure, c);
  uint32_t status;

  if (!info->want_reply
      && parse_uint32(args, &status)
      && parse_eod(args))
    {
      verbose("client.c: Receiving exit-status %i on channel %i\n",
	      status, channel->channel_number);

      *closure->exit_status = status;
      ALIST_SET(channel->request_types, ATOM_EXIT_STATUS, NULL);
      ALIST_SET(channel->request_types, ATOM_EXIT_SIGNAL, NULL);
      
      COMMAND_RETURN(s, channel);
    }
  else
    /* Invalid request */
    PROTOCOL_ERROR(e, "Invalid exit-status message");
}

static void
do_exit_signal(struct channel_request *c,
	       struct ssh_channel *channel,
	       struct channel_request_info *info,
	       struct simple_buffer *args,
	       struct command_continuation *s,
	       struct exception_handler *e)
{
  CAST(exit_handler, closure, c);

  uint32_t signal;
  int core;

  const uint8_t *msg;
  uint32_t length;

  const uint8_t *language;
  uint32_t language_length;
  
  if (!info->want_reply
      && parse_atom(args, &signal)
      && parse_boolean(args, &core)
      && parse_string(args, &length, &msg)
      && parse_string(args, &language_length, &language)
      && parse_eod(args))
    {
      /* FIXME: What exit status should be returned when the remote
       * process dies violently? */

      *closure->exit_status = 7;

      werror("Remote process was killed by signal: %ups %z\n",
	     length, msg,
	     core ? "(core dumped remotely)\n": "");
      
      ALIST_SET(channel->request_types, ATOM_EXIT_STATUS, NULL);
      ALIST_SET(channel->request_types, ATOM_EXIT_SIGNAL, NULL);

      COMMAND_RETURN(s, channel);
    }
  else
    /* Invalid request */
    PROTOCOL_ERROR(e, "Invalid exit-signal message");
}

struct channel_request *
make_handle_exit_status(int *exit_status)
{
  NEW(exit_handler, self);

  self->super.handler = do_exit_status;

  self->exit_status = exit_status;

  return &self->super;
}

struct channel_request *
make_handle_exit_signal(int *exit_status)
{
  NEW(exit_handler, self);

  self->super.handler = do_exit_signal;

  self->exit_status = exit_status;

  return &self->super;
}

/* GABA:
   (class
     (name session_open_command)
     (super channel_open_command)
     (vars
       ; This command can only be executed once,
       ; so we can allocate the session object in advance.
       (session object ssh_channel)))
*/

static struct ssh_channel *
new_session(struct channel_open_command *s,
	    struct ssh_connection *connection,
	    uint32_t local_channel_number,
	    struct lsh_string **request)
{
  CAST(session_open_command, self, s);
  struct ssh_channel *res;

  self->session->connection = connection;
  
  *request = format_channel_open(ATOM_SESSION,
				 local_channel_number,
				 self->session, "");
  
  res = self->session;

  /* Make sure this command can not be invoked again */
  self->session = NULL;

  return res;
}

struct command *
make_open_session_command(struct ssh_channel *session)
{
  NEW(session_open_command, self);
  self->super.super.call = do_channel_open_command;
  self->super.new_channel = new_session;
  self->session = session;

  return &self->super.super;
}


static struct lsh_string *
do_format_shell_request(struct channel_request_command *s UNUSED,
			struct ssh_channel *channel,
			struct command_continuation **c)
{
  return format_channel_request(ATOM_SHELL, channel, !!*c, "");
}

struct channel_request_command request_shell =
{ { STATIC_HEADER, do_channel_request_command }, do_format_shell_request };


/* GABA:
   (class
     (name exec_request)
     (super channel_request_command)
     (vars
       (command string)))
*/

static struct lsh_string *
do_format_exec_request(struct channel_request_command *s,
		       struct ssh_channel *channel,
		       struct command_continuation **c)
{
  CAST(exec_request, self, s);

  verbose("lsh: Requesting remote exec.\n");

  return format_channel_request(ATOM_EXEC, channel,
				!!*c, "%S", self->command);
}

struct command *
make_exec_request(struct lsh_string *command)
{
  NEW(exec_request, req);

  req->super.format_request = do_format_exec_request;
  req->super.super.call = do_channel_request_command;
  req->command = command;

  return &req->super.super;
}


/* GABA:
   (class
     (name subsystem_request)
     (super channel_request_command)
     (vars
       (subsystem string)))
*/

static struct lsh_string *
do_format_subsystem_request(struct channel_request_command *s,
			    struct ssh_channel *channel,
			    struct command_continuation **c)
{
  CAST(subsystem_request, self, s);

  verbose("lsh: Requesting remote subsystem.\n");

  return format_channel_request(ATOM_SUBSYSTEM, channel,
				!!*c, "%S", self->subsystem);
}

struct command *
make_subsystem_request(struct lsh_string *subsystem)
{
  NEW(subsystem_request, req);

  req->super.format_request = do_format_subsystem_request;
  req->super.super.call = do_channel_request_command;
  req->subsystem = subsystem;

  return &req->super.super;
}


/* Handling of options and operations shared by the plain lsh client
 * and lshg. */

/* Forward declaration */

static struct ssh_channel *
make_client_session(struct client_options *options);

/* Block size for stdout and stderr buffers */
#define BLOCK_SIZE 32768

/* Window size for the session channel
 *
 * NOTE: Large windows seem to trig a bug in sshd2. */
#define WINDOW_SIZE 10000

#define ARG_NOT 0x400

#define OPT_STDIN 0x210
#define OPT_STDOUT 0x211
#define OPT_STDERR 0x212

#define OPT_SUBSYSTEM 0x214
#define OPT_DETACH 0x215

#define OPT_ASKPASS 0x216

#define OPT_WRITE_PID 0x217

void
init_client_options(struct client_options *self,
		    struct randomness *random,
		    struct exception_handler *handler,
		    int *exit_code)			 
{
  self->random = random;

  self->tty = make_unix_interact();
  self->escape = -1;
  
  self->handler = handler;

  self->exit_code = exit_code;
  
  self->not = 0;
  self->port = NULL;
  self->target = NULL;

  USER_NAME_FROM_ENV(self->user);
  self->local_user = self->user;

  self->with_remote_peers = 0; 
  self->with_pty = -1;
  self->with_x11 = 0;
    
  self->stdin_file = NULL;
  self->stdout_file = NULL;
  self->stderr_file = NULL;

  self->used_stdin = 0;
  self->used_pty = 0;
  self->used_x11 = 0;
  
  self->detach_end = 0;
  self->write_pid = 0;
  
  self->start_shell = 1;
  self->remote_forward = 0;

  self->inhibit_actions = 0;

  object_queue_init(&self->actions);
  
  self->resources = make_resource_list();
  gc_global(&self->resources->super);
}

static const struct argp_option
client_options[] =
{
  /* Name, key, arg-name, flags, doc, group */
  { "port", 'p', "Port", 0, "Connect to this port.", 0 },
  { "user", 'l', "User name", 0, "Login as this user.", 0 },
  { "askpass", OPT_ASKPASS, "Program", 0,
    "Program to use for reading passwords. "
    "Should be an absolute filename.", 0 },
  { NULL, 0, NULL, 0, "Actions:", CLIENT_ARGP_ACTION_GROUP },
  { "forward-local-port", 'L', "local-port:target-host:target-port", 0, "", 0 },
  { "forward-socks", 'D', "port", OPTION_ARG_OPTIONAL, "Enable socks dynamic forwarding", 0 },
#if 0
  { "forward-remote-port", 'R', "remote-port:target-host:target-port", 0, "", 0 },
#endif
  { "nop", 'N', NULL, 0, "No operation (suppresses the default action, "
    "which is to spawn a remote shell)", 0 },
  { "background", 'B', NULL, 0, "Put process into the background. Implies -N.", 0 },
  { "execute", 'E', "command", 0, "Execute a command on the remote machine", 0 },
  { "shell", 'S', "command", 0, "Spawn a remote shell", 0 },
  { "subsystem", OPT_SUBSYSTEM, "subsystem-name", 0,
#if WITH_PTY_SUPPORT 
    "Connect to given subsystem. Implies --no-pty.",
#else
    "Connect to given subsystem.",
#endif
    0 },
  /* { "gateway", 'G', NULL, 0, "Setup a local gateway", 0 }, */
  { NULL, 0, NULL, 0, "Universal not:", 0 },
  { "no", 'n', NULL, 0, "Inverts the effect of the next modifier", 0 },

  { NULL, 0, NULL, 0, "Modifiers that apply to port forwarding:",
    CLIENT_ARGP_MODIFIER_GROUP - 10 },
  { "remote-peers", 'g', NULL, 0, "Allow remote access to forwarded ports", 0 },
  { "no-remote-peers", 'g' | ARG_NOT, NULL, 0, 
    "Disallow remote access to forwarded ports (default).", 0 },

  { NULL, 0, NULL, 0, "Modifiers that apply to remote execution:", 0 },
  { "stdin", OPT_STDIN, "Filename", 0, "Redirect stdin", 0},
  { "no-stdin", OPT_STDIN | ARG_NOT, NULL, 0, "Redirect stdin from /dev/null", 0}, 
  { "stdout", OPT_STDOUT, "Filename", 0, "Redirect stdout", 0},
  { "no-stdout", OPT_STDOUT | ARG_NOT, NULL, 0, "Redirect stdout to /dev/null", 0}, 
  { "stderr", OPT_STDERR, "Filename", 0, "Redirect stderr", 0},
  { "no-stderr", OPT_STDERR | ARG_NOT, NULL, 0, "Redirect stderr to /dev/null", 0}, 

  { "detach", OPT_DETACH, NULL, 0, "Detach from terminal at session end.", 0},
  { "no-detach", OPT_DETACH | ARG_NOT, NULL, 0, "Do not detach session at end," 
    " wait for all open channels (default).", 0},

#if WITH_PTY_SUPPORT
  { "pty", 't', NULL, 0, "Request a remote pty (default).", 0 },
  { "no-pty", 't' | ARG_NOT, NULL, 0, "Don't request a remote pty.", 0 },
#endif /* WITH_PTY_SUPPORT */
  { NULL, 0, NULL, 0, "Miscellaneous options:", 0 },
  { "escape-char", 'e', "Character", 0, "Escape char. `none' means disable. "
    "Default is to use `~' if we have a tty, otherwise none.", 0 },
  { "write-pid", OPT_WRITE_PID, NULL, 0, "Make -B write the pid of the backgrounded "
    "process to stdout.", 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};


/* GABA:
   (expr
     (name make_start_session)
     (params
       (open_session object command)
       (requests object object_list))
     (expr (lambda (connection)
       ((progn requests)
         ; Create a "session" channel
         (open_session connection)))))
*/

/* Requests a shell or command, and connects the channel to our stdio. */
/* GABA:
   (expr
     (name client_start_session)
     (params
       (request object command))
     (expr
       (lambda (session)
          (client_start_io (request session)))))
*/

static struct command *
make_client_start_session(struct command *request)
{
  CAST_SUBTYPE(command, r, client_start_session(request));
  return r;
}

static void
client_maybe_pty(struct client_options *options,
		 int default_pty,
		 struct object_queue *q)
{
#if WITH_PTY_SUPPORT
  int with_pty = options->with_pty;
  if (with_pty < 0)
    with_pty = default_pty;
  
  if (with_pty && !options->used_pty)
    {
      options->used_pty = 1;
      
      if (options->tty && INTERACT_IS_TTY(options->tty))
	{
	  struct command *get_pty = make_pty_request(options->tty);

	  if (get_pty)
	    object_queue_add_tail(q,
				  /* Ignore EXC_CHANNEL_REQUEST for the pty allocation call. */
				  &make_catch_apply
				  (make_catch_handler_info(EXC_ALL, EXC_CHANNEL_REQUEST,
							   0, NULL),
				   get_pty)->super);
	  else
	    werror("lsh: Can't use tty (probably getattr or atexit failed).\n");
	}
      else
	werror("lsh: No tty available.\n");
    }
#endif
}

static void
client_maybe_x11(struct client_options *options,
		 struct object_queue *q)
{
  if (options->with_x11)
    {
      char *display = getenv(ENV_DISPLAY);
      struct command *request = NULL;
      
      assert(options->random);
      if (display)
	request = make_forward_x11(display, options->random);
	  
      if (request)
	{
	  object_queue_add_tail(q, &request->super);
	  options->used_x11 = 1;
	}
      else
	werror("Can't find any local X11 display to forward.\n");
    }
}

/* Create an interactive session */
static struct command *
client_shell_session(struct client_options *options)
{
  struct ssh_channel *session = make_client_session(options);
  
  if (session)
    {
      struct object_queue session_requests;

      object_queue_init(&session_requests);

      client_maybe_pty(options, 1, &session_requests);
      client_maybe_x11(options, &session_requests);
  
      object_queue_add_tail(&session_requests,
			    &make_client_start_session(&request_shell.super)->super);
  
      {
	CAST_SUBTYPE(command, r,
		     make_start_session
		     (make_open_session_command(session),
		      queue_to_list_and_kill(&session_requests)));
	return r;
      }
    }
  else
    return NULL;
}

/* Create a session for a subsystem */
static struct command *
client_subsystem_session(struct client_options *options,
			 struct lsh_string *subsystem)
{
  struct ssh_channel *session = make_client_session(options);
  
  if (session)
    {
      CAST_SUBTYPE(command, r,
		   make_start_session
		   (make_open_session_command(session),
		    make_object_list(1,
				     make_client_start_session
				     (make_subsystem_request(subsystem)),
				     -1)));
      return r;
    }
  
  return NULL;
}

/* Create a session executing a command line */
static struct command *
client_command_session(struct client_options *options,
		       struct lsh_string *command)
{
  struct ssh_channel *session = make_client_session(options);
  
  if (session)
    {
      struct object_queue session_requests;
    
      object_queue_init(&session_requests);
  
      /* NOTE: Doesn't ask for a pty by default. That's traditional
       * behaviour, although perhaps not the Right Thing. */
      
      client_maybe_pty(options, 0, &session_requests);
      client_maybe_x11(options, &session_requests);

      object_queue_add_tail(&session_requests,
			    &make_client_start_session(make_exec_request(command))->super);
      {
	CAST_SUBTYPE(command, r,
		     make_start_session
		     (make_open_session_command(session),
		      queue_to_list_and_kill(&session_requests)));
	return r;
      }
    }
  
  return NULL;
}

struct command *
client_add_action(struct client_options *options,
		  struct command *action)
{
  if (action)
    object_queue_add_tail(&options->actions, &action->super);

  return action;
}

struct command *
client_prepend_action(struct client_options *options,
		      struct command *action)
{
  if (action)
    object_queue_add_head(&options->actions, &action->super);

  return action;
}

/* NOTE: Some of the original quoting is lost here. */
static struct lsh_string *
rebuild_command_line(unsigned argc, char **argv)
{
  unsigned length;
  unsigned i;
  unsigned pos;
  struct lsh_string *r;
  unsigned *alengths = alloca(sizeof(unsigned) * argc);
  
  assert (argc);
  length = argc - 1; /* Number of separating spaces. */

  for (i = 0; i<argc; i++)
    {
      alengths[i] = strlen(argv[i]);
      length += alengths[i];
    }

  r = lsh_string_alloc(length);
  lsh_string_write(r, 0, alengths[0], argv[0]);
  pos = alengths[0];
  for (i = 1; i<argc; i++)
    {
      lsh_string_putc(r, pos++, ' ');
      lsh_string_write(r, pos, alengths[i], argv[i]);
      pos += alengths[i];
    }

  assert(pos == length);

  return r;
}

/* A callback that exits the process immediately. */
DEFINE_ESCAPE(exit_callback, "Exit.")
{
  exit(EXIT_SUCCESS);
}

DEFINE_ESCAPE(verbose_callback, "Toggle verbose messages.")
{
  verbose_flag = !verbose_flag;
  if (verbose_flag)
    verbose("Enabling verbose messages\n");
}

DEFINE_ESCAPE(debug_callback, "Toggle debug messages.")
{
  debug_flag = !debug_flag;
  if (debug_flag)
    debug("Enabling debug messages\n");
}

DEFINE_ESCAPE(quiet_callback, "Toggle warning messages.")
{
  quiet_flag = !quiet_flag;
  if (!quiet_flag)
    werror("Enabling warning messages\n");
}

/* GABA:
   (class
     (name background_process_command)
     (super command)
     (vars
       (write_pid . int)))
*/

static void
do_background_process(struct command *s,
		      struct lsh_object *a,
		      struct command_continuation *c,
		      struct exception_handler *e UNUSED)
{
  CAST(background_process_command, self, s);
  pid_t pid;
  
  trace("do_background_process\n");
  
  pid = fork();
  
  switch (pid)
    {
    case 0:
      /* Child */
      /* FIXME: Should we create a new process group, close our tty
       * and stdio, etc? */
      COMMAND_RETURN(c, a);
      break;
    case -1:
      /* Error */
      werror("background_process: fork failed %e\n", errno);
      COMMAND_RETURN(c, a);
      break;
    default:
      /* Parent */
      if (self->write_pid)
	{
	  struct lsh_string *msg = ssh_format("%di\n", pid);
	  const struct exception *e = write_raw (STDOUT_FILENO, STRING_LD(msg));

	  if (e)
	    werror ("Write to stdout failed!?: %z\n", e->msg);
	}
      _exit(EXIT_SUCCESS);
    }
}

static struct command *
make_background_process(int write_pid)
{
  NEW(background_process_command, self);

  self->super.call = do_background_process;
  self->write_pid = write_pid;

  return &self->super;
}

/* Create a session object. stdout and stderr are shared (although
 * with independent lsh_fd objects). stdin can be used by only one
 * session (until something "session-control"/"job-control" is added).
 * */
static struct ssh_channel *
make_client_session(struct client_options *options)
{
  int in;
  int out;
  int err;
  enum io_type in_type = IO_NORMAL;
  enum io_type out_type = IO_NORMAL;
  enum io_type err_type = IO_NORMAL;
  
  int is_tty = 0;
  struct ssh_channel *session;
  
  struct escape_info *escape = NULL;
  struct lsh_callback *detach_cb = NULL;
  
  debug("lsh.c: Setting up stdin\n");

  if (options->stdin_file)
    in = open(options->stdin_file, O_RDONLY);
      
  else
    {
      if (options->used_stdin)
	in = open("/dev/null", O_RDONLY);
      else 
	{
	  in = STDIN_FILENO;
	  in_type = IO_STDIO;
	  is_tty = isatty(STDIN_FILENO);
	  
	  options->used_stdin = 1;
	}
    }

  if (in < 0)
    {
      werror("lsh: Can't open stdin %e\n", errno);
      return NULL;
    }

  /* Attach the escape char handler, if appropriate. */
  if (options->escape > 0)
    {
      verbose("Enabling explicit escape character `%pc'\n",
	      options->escape);
      escape = make_escape_info(options->escape);
    }
  else if ( (options->escape < 0) && is_tty)
    {
      verbose("Enabling default escape character `%pc'\n",
	      DEFAULT_ESCAPE_CHAR);
      escape = make_escape_info(DEFAULT_ESCAPE_CHAR);
    }

  /* Bind ^Z to suspend. */
  if (escape)
    {
      /* Bind ^Z to suspend. */
      escape->dispatch[26] = &suspend_callback;
      escape->dispatch['.'] = &exit_callback;

      /* Toggle the verbosity flags */
      escape->dispatch['d'] = &debug_callback;
      escape->dispatch['v'] = &verbose_callback;
      escape->dispatch['q'] = &quiet_callback;      
    }
  
  debug("lsh.c: Setting up stdout\n");

  if (options->stdout_file)
    /* FIXME: Use O_TRUNC too? */
    out = open(options->stdout_file, O_WRONLY | O_CREAT, 0666);
  else
    {
      out_type = IO_STDIO;
      out = STDOUT_FILENO;
    }
  if (out < 0)
    {
      werror("lsh: Can't open stdout %e\n", errno);
      close(in);
      return NULL;
    }

  debug("lsh.c: Setting up stderr\n");
  
  if (options->stderr_file)
    /* FIXME: Use O_TRUNC too? */
    err = open(options->stderr_file, O_WRONLY | O_CREAT, 0666);
  else
    {
      err_type = IO_STDERR;
      err = STDERR_FILENO;
    }
  if (err < 0) 
    {
      werror("lsh: Can't open stderr!\n");
      close(in);
      close(out);
      return NULL;
    }

  if (options->detach_end) /* Detach? */
    detach_cb = make_detach_callback(options->exit_code);  

  /* Clear options */
  options->stdin_file = options->stdout_file = options->stderr_file = NULL;

  session = make_client_session_channel
    (io_read(make_lsh_fd(in, in_type, "client stdin", options->handler),
	     NULL, NULL),
     io_write(make_lsh_fd(out, out_type, "client stdout", options->handler),
	      BLOCK_SIZE, 
	      detach_cb),
     io_write(make_lsh_fd(err, err_type, "client stderr", options->handler),
	      BLOCK_SIZE, NULL),
     escape,
     WINDOW_SIZE,
     options->exit_code);
  
  if (options->detach_end)
    {
      remember_resource(session->resources, make_detach_resource(detach_cb));
      options->detach_end = 0;
    }

  /* The channel won't get registered in any other resource_list until
   * later, so we must register it here to be able to clean up
   * properly if the connection fails early. */
  remember_resource(options->resources, &session->resources->super);
  
  return session;
}


/* Treat environment variables as sources for options */

void
envp_parse(const struct argp *argp,
	   const char** envp,
	   const char* name,
	   unsigned flags, 
	   void *input)
{
  CAST_SUBTYPE(client_options, options, input);
  int nlen = strlen(name);

  while (*envp)
    {
      if (!strncmp(*envp, name, nlen)) 	  /* Matching environment entry */
	{
	  char** sim_argv;
	  char* entry;

	  /* Make a copy we can modify */
	  
	  entry = strdup(*envp+nlen); 	  /* Skip variable name */

	  if (entry)
	    {
	      /* Extra space doesn't hurt */
	      sim_argv = malloc(sizeof(char*) * (strlen(entry)+2));

	      if (sim_argv)
		{
		  int sim_argc = 1;
		  char *token = strtok(entry, " \n\t");
		  
		  sim_argv[0] = "";

		  while (token) /* For all tokens in variable */
		    {
		      sim_argv[sim_argc++] = token;
		      token = strtok( NULL, " \n\t");
		    }

		  sim_argv[sim_argc] = NULL;
	
		  options->inhibit_actions = 1; /* Disable nnormal actions performed at end */
		  argp_parse(argp, sim_argc, sim_argv, flags | ARGP_NO_ERRS | ARGP_NO_EXIT, NULL, input);
		  options->inhibit_actions = 0; /* Reenable */
		}
	    }
	}

      envp++; 
   }
}

/* Parse the argument for -R and -L */
int
client_parse_forward_arg(char *arg,
			 uint32_t *listen_port,
			 struct address_info **target)
{
  char *first;
  char *second;
  char *end;
  long port;
  
  first = strchr(arg, ':');
  if (!first)
    return 0;

  second = strchr(first + 1, ':');
  if (!second || (second == first + 1))
    return 0;

  if (strchr(second + 1, ':'))
    return 0;

  port = strtol(arg, &end, 0);
  if ( (end == arg)  || (end != first)
       || (port < 0) || (port > 0xffff) )
    return 0;

  *listen_port = port;

  port = strtol(second + 1, &end, 0);
  if ( (end == second + 1) || (*end != '\0')
       || (port < 0) || (port > 0xffff) )
    return 0;

  *target = make_address_info(ssh_format("%ls", second - first - 1, first + 1), port);
  
  return 1;
}

static int
client_arg_unsigned(const char *arg, unsigned long *n)
{
  char *end;
  if (*arg == 0)
    return 0;

  *n = strtoul(arg, &end, 0);
  return *end == 0;
}
		    
#define CASE_ARG(opt, attr, none)		\
  case opt:					\
    if (options->not)				\
      {						\
        options->not = 0;			\
						\
      case opt | ARG_NOT:			\
        options->attr = none;			\
        break;					\
      }						\
      						\
    options->attr = arg;			\
    break

#define CASE_FLAG(opt, flag)			\
  case opt:					\
    if (options->not)				\
      {						\
        options->not = 0;			\
						\
      case opt | ARG_NOT:			\
        options->flag = 0;			\
        break;					\
      }						\
      						\
    options->flag = 1;				\
    break

static error_t
client_argp_parser(int key, char *arg, struct argp_state *state)
{
  CAST_SUBTYPE(client_options, options, state->input);

  switch(key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    case ARGP_KEY_NO_ARGS:
      argp_usage(state);
      break;
    case ARGP_KEY_ARG:
      if (!state->arg_num)
	options->target = arg;
      
      else
	/* Let the next case parse it.  */
	return ARGP_ERR_UNKNOWN;

      break;
    case ARGP_KEY_ARGS:
      client_add_action
	(options,
	 client_command_session
	 (options, rebuild_command_line(state->argc - state->next,
				     state->argv + state->next)));
      options->start_shell = 0;
      break;

    case ARGP_KEY_END:
      if (options->inhibit_actions)
	break;

      if (!options->user)
	{
	  argp_error(state, "No user name given. Use the -l option, or set LOGNAME in the environment.");
	  break;
	}

#if WITH_TCP_FORWARD
      if (options->remote_forward)
	client_add_action(options,
			  make_install_fix_channel_open_handler
			  (ATOM_FORWARDED_TCPIP, &channel_open_forwarded_tcpip));
#endif /* WITH_TCP_FORWARD */

      /* Add shell action */
      if (options->start_shell)
	client_add_action(options, client_shell_session(options));

      if (options->used_x11)
	client_add_action(options,
			  make_install_fix_channel_open_handler
			  (ATOM_X11,
			   &channel_open_x11));
      
      /* Install suspend-handler */
      suspend_install_handler();
      break;

    case 'p':
      options->port = arg;
      break;

    case 'l':
      options->user = arg;
      break;

    case OPT_ASKPASS:
      INTERACT_SET_ASKPASS(options->tty, arg);
      break;
      
    case 'e':
      if (arg[0] && !arg[1])
	/* A single char argument */
	options->escape = arg[0];
      else if (!strcasecmp(arg, "none"))
	options->escape = 0;
      else
	argp_error(state, "Invalid escape char: `%s'. "
		   "You must use a single character or `none'.", arg);
      break;
    case 'E':
      client_add_action(options,
			client_command_session(options,
					       ssh_format("%lz", arg)));
      break;

    case 'S':
      client_add_action(options, client_shell_session(options));
      break;

    case OPT_SUBSYSTEM:
      client_add_action(options,
			client_subsystem_session(options,
						 ssh_format("%lz", arg)));

      options->start_shell = 0;
#if WITH_PTY_SUPPORT
      options->with_pty = 0;
#endif
      break;

    case 'L':
      {
	uint32_t listen_port;
	struct address_info *target;

	if (!client_parse_forward_arg(arg, &listen_port, &target))
	  argp_error(state, "Invalid forward specification `%s'.", arg);

	client_add_action(options, make_forward_local_port
			  (make_address_info((options->with_remote_peers
					      ? NULL
					      : ssh_format("%lz", "127.0.0.1")),
					     listen_port),
			   target));
	break;
      }      

    case 'D':
      {
	unsigned long socks_port = DEFAULT_SOCKS_PORT;
	if (arg && (client_arg_unsigned(arg, &socks_port) == 0 || socks_port > 0xffff))
	  argp_error(state, "Invalid port number `%s' for socks.", arg);

	client_add_action(options, make_socks_server
			  (make_address_info((options->with_remote_peers
					      ? NULL
					      : ssh_format("%lz", "127.0.0.1")),
					     socks_port)));
	break;
      }
      
    case 'N':
      options->start_shell = 0;
      break;

    case 'B':
      options->start_shell = 0;
      client_add_action(options, make_background_process(options->write_pid));
      break;
      
    CASE_FLAG('g', with_remote_peers);

#if WITH_PTY_SUPPORT
    CASE_FLAG('t', with_pty);
#endif /* WITH_PTY_SUPPORT */

    CASE_FLAG(OPT_DETACH, detach_end);
    CASE_FLAG(OPT_WRITE_PID, write_pid);
    
    CASE_ARG(OPT_STDIN, stdin_file, "/dev/null");
    CASE_ARG(OPT_STDOUT, stdout_file, "/dev/null"); 
    CASE_ARG(OPT_STDERR, stderr_file, "/dev/null");

    case 'n':
      options->not = !options->not;
      break;
    }
  return 0;
}

const struct argp client_argp =
{
  client_options,
  client_argp_parser,
  NULL, NULL, NULL, NULL, NULL
};
