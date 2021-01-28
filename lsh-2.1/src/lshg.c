/* lshg.c
 *
 * Connect to a gateway. */

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
#include <locale.h>
#include <unistd.h>

#include "charset.h"
#include "client.h"
#include "connection.h"
#include "format.h"
#include "gateway.h"
#include "gateway_commands.h"
#include "interact.h"
#include "io_commands.h"
#include "ssh.h"
#include "version.h"
#include "werror.h"
#include "xalloc.h"

#include "lsh_argp.h"

/* Forward declarations */
struct command options2info;
#define OPTIONS2INFO (&options2info.super)

/* Until configure finds this */

#ifndef LSH_FILENAME
#define LSH_FILENAME "lsh"
#endif

#include "lshg.c.x"

/* GABA:
   (class
     (name lshg_options)
     (super client_options)
     (vars
       (gateway object local_info)
       (fallback_lsh . int )))
*/

static struct lshg_options *
make_options(struct exception_handler *handler,
	     int *exit_code) 
{
  NEW(lshg_options, self);
  init_client_options(&self->super, NULL, handler, exit_code);

  self->gateway = NULL;
  self->fallback_lsh = 0;

  return self;
}

DEFINE_COMMAND(options2info)
     (struct command *s UNUSED,
      struct lsh_object *a,
      struct command_continuation *c,
      struct exception_handler *e UNUSED)
{
  CAST(lshg_options, self, a);

  COMMAND_RETURN(c, self->gateway);
}


/* GABA:
   (expr
     (name make_lshg_connect)
     (params
       (resource object resource)
       (actions object object_list))
     (expr
       (lambda (options)
         ((progn actions)
	  (protect resource gateway_init
	    (connect_local (options2info options)))))))
*/

/* GABA:
   (class
     (name lshg_simple_action)
     (super command)
     (vars
       (msg . "const char *")))
*/

static void
do_lshg_send_debug(struct command *s,
		   struct lsh_object *x,
		   struct command_continuation *c UNUSED,
		   struct exception_handler *e UNUSED)
{
  CAST(lshg_simple_action, self, s);
  CAST(ssh_connection, connection, x);

  send_debug_message(connection, self->msg, 1);
}

static struct command *
make_lshg_send_debug(const char *msg)
{
  NEW(lshg_simple_action, self);
  self->msg = msg;
  self->super.call = do_lshg_send_debug;

  return &self->super;
}

static void
do_lshg_send_ignore(struct command *s,
		    struct lsh_object *x,
		    struct command_continuation *c UNUSED,
		    struct exception_handler *e UNUSED)
{
  CAST(lshg_simple_action, self, s);
  CAST(ssh_connection, connection, x);

  /* Can be sent even during key exchange. */
  connection_send_kex(connection, ssh_format("%c%z", SSH_MSG_IGNORE, self->msg));
}

static struct command *
make_lshg_send_ignore(const char *msg)
{
  NEW(lshg_simple_action, self);
  self->msg = msg;
  self->super.call = do_lshg_send_ignore;

  return &self->super;
}


/* Option parsing */

const char *argp_program_version
= "lshg-" VERSION ", secsh protocol version " CLIENT_PROTOCOL_VERSION;

const char *argp_program_bug_address = BUG_ADDRESS;

#define ARG_SEND_DEBUG 0x200
#define ARG_SEND_IGNORE 0x201

static const struct argp_option
main_options[] =
{
  /* Name, key, arg-name, flags, doc, group */
  { "send-debug", ARG_SEND_DEBUG, "Message", 0, "Send a debug message "
    "to the remote machine.", CLIENT_ARGP_ACTION_GROUP },
  { "send-ignore", ARG_SEND_IGNORE, "Message", 0, "Send an ignore message "
    "to the remote machine.", 0 },
  { NULL, 'G', 0, 0, "If no usable gateway is found, "
    "launch lsh instead.", 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static const struct argp_child
main_argp_children[] =
{
  { &client_argp, 0, "", 0 },
  { &werror_argp, 0, "", 0 },
  { NULL, 0, NULL, 0}
};


static error_t
main_argp_parser(int key, char *arg, struct argp_state *state)
{
  CAST(lshg_options, self, state->input);
  
  switch(key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    case ARGP_KEY_INIT:
      state->child_inputs[0] = &self->super;
      state->child_inputs[1] = NULL;
      break;

    case ARGP_KEY_END:
      if (self->super.inhibit_actions)
	break;

      if (!self->super.local_user)
	{
	  argp_error(state, "You have to set LOGNAME in the environment.");
	  break;
	}
      assert(self->super.user);
      assert(self->super.target);
      
      if (object_queue_is_empty(&self->super.actions))
	{
	  argp_error(state, "No actions given.");
	  break;
	}

      self->gateway = make_gateway_address(self->super.local_user,
					   self->super.user,
					   self->super.target);

      if (!self->gateway)
	{
	  argp_error(state, "Local or remote user name, or the target host name, are too "
		     "strange for the gateway socket name construction.");
	  break;
	}
      break;

    case ARG_SEND_DEBUG:
      client_add_action(&self->super, make_lshg_send_debug(arg));
      break;

    case ARG_SEND_IGNORE:
      client_add_action(&self->super, make_lshg_send_ignore(arg));
      break;
      
    case 'G':
      self->fallback_lsh = 1;
      break;
    }
  return 0;
}

static const struct argp
main_argp =
{ main_options, main_argp_parser, 
  "host\nhost command",
  "Connects to a remote machine, using a gateway\v"
  "Connects to the remote machine, using a local gateway, previously setup "
  "by running lsh -G.",
  main_argp_children,
  NULL, NULL
};

/* GABA: 
   (class
     (name lshg_exception_handler)
     (super exception_handler)
     (vars
       (argv . "char**")
       (fallback_lsh . int)))
*/

static void
do_exc_lshg_handler(struct exception_handler *s,
		    const struct exception *e)
{
  CAST(lshg_exception_handler, self, s);

  if(e->type == EXC_IO_CONNECT &&
     self->fallback_lsh)
    {
      verbose("No usable gateway found, launching lsh instead.\n");
      execvp(LSH_FILENAME, self->argv); /* FIXME: filename */

      werror("lsh launch failed, giving up.\n");
      exit(EXIT_FAILURE);
    }

  werror("%z\n", e->msg);

  if(e->type == EXC_IO_CONNECT)
    {
      werror("Exiting...\n");
      exit(EXIT_FAILURE);
    }
}

static struct exception_handler *
make_lshg_exception_handler(struct exception_handler *parent,
			    const char *context,
			    char **argv,
			    int fallback_lsh)
{
  NEW(lshg_exception_handler, self);
  self->super.raise = do_exc_lshg_handler;
  self->super.parent = parent;
  self->super.context = context;
  self->argv = argv;
  self->fallback_lsh = fallback_lsh;
  return &self->super;
}

int
main(int argc, char **argv, const char** envp)
{
  struct lshg_options *options;

  int exit_code = 17;

  io_init();
  
  /* For filtering messages. Could perhaps also be used when converting
   * strings to and from UTF8. */
  setlocale(LC_CTYPE, "");

  /* FIXME: Choose character set depending on the locale */
  set_local_charset(CHARSET_LATIN1);

  options = make_options(make_lshg_exception_handler
			 (&default_exception_handler,
			  HANDLER_CONTEXT,
			  NULL,
			  0),
			 &exit_code);

  envp_parse(&main_argp, envp, "LSHGFLAGS=", ARGP_IN_ORDER, options);
  argp_parse(&main_argp, argc, argv, ARGP_IN_ORDER, NULL, options);

  {
    CAST_SUBTYPE(command, lshg_connect,
		 make_lshg_connect(&options->super.resources->super,
				   queue_to_list(&options->super.actions)));

    COMMAND_CALL(lshg_connect, options, &discard_continuation,
		 make_lshg_exception_handler(&default_exception_handler,
					     HANDLER_CONTEXT,
					     argv,
					     options->fallback_lsh));
  }

  io_run();

  /* Close all files and other resources associated with the backend. */
  io_final();
  
  return exit_code;
}
