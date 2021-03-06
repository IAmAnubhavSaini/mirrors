/* client_userauth.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1999 Bal�zs Scheidler, Niels M�ller
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

#include "client_userauth.h"

#include "charset.h"
#include "command.h"
#include "format.h"
#include "lsh_string.h"
#include "parse.h"
#include "publickey_crypto.h"
#include "ssh.h"
#include "werror.h"
#include "xalloc.h"

/* Forward declaration */
struct client_userauth; 

#include "client_userauth.c.x"

static struct lsh_string *
format_userauth_none(struct lsh_string *name,
                     int service)
{
  return ssh_format("%c%S%a%a",
		    SSH_MSG_USERAUTH_REQUEST,
		    name,
		    service,
		    ATOM_NONE);
}

/* The last parameters says whether or not to free the password. */
struct lsh_string *
format_userauth_password(struct lsh_string *name,
			 int service,
			 struct lsh_string *passwd,
			 int free)
{
  return ssh_format(free ? "%c%S%a%a%c%fS" : "%c%S%a%a%c%S",
		    SSH_MSG_USERAUTH_REQUEST,
		    name,
		    service,
		    ATOM_PASSWORD,
		    0,
		    passwd);
}

static struct lsh_string *
format_userauth_publickey_query(struct lsh_string *name,
				uint32_t service,
				uint32_t keytype,
				struct lsh_string *public)
{
  return ssh_format("%c%S%a%a%c%a%S",
		    SSH_MSG_USERAUTH_REQUEST,
		    name,
		    service,
		    ATOM_PUBLICKEY,
		    0,
		    keytype,
		    public);
}

static struct lsh_string *
format_userauth_publickey(struct lsh_string *name,
			  uint32_t service,
			  uint32_t keytype,
			  struct lsh_string *public)
{
  return ssh_format("%c%S%a%a%c%a%S",
		    SSH_MSG_USERAUTH_REQUEST,
		    name,
		    service,
		    ATOM_PUBLICKEY,
		    1,
		    keytype,
		    public);
}

static struct lsh_string *
format_userauth_kbdinteract(struct lsh_string *name, uint32_t service)
{
  struct lsh_string *s;
  s = ssh_format("%c%S%a%a%i%i",
		 SSH_MSG_USERAUTH_REQUEST,
		 name, service,
		 /* Empty language tag and submethods */
		 ATOM_KEYBOARD_INTERACTIVE, 0, 0);
  debug("format_userauth_kbdinteract %xS\n", s);
  return s;
}

static struct lsh_string *
format_userauth_info_response(struct interact_dialog *dialog)
{
  uint32_t length;
  unsigned i;
  struct lsh_string *msg;
  uint32_t p;

  /* We need to format a message containing a variable number of
     strings. */

  /* First convert to utf8 */
  for (i = 0; i < dialog->nprompt; i++)
    dialog->response[i] = local_to_utf8(dialog->response[i], 1);
  
  for (i = length = 0; i < dialog->nprompt; i++)
    length += lsh_string_length(dialog->response[i]);

  msg = ssh_format("%c%i%lr", SSH_MSG_USERAUTH_INFO_RESPONSE, dialog->nprompt,
		   length + 4 * dialog->nprompt, &p);

  for (i = 0; i < dialog->nprompt; i++)
    {
      struct lsh_string *r = dialog->response[i];
      uint32_t rlength = lsh_string_length(r);
      lsh_string_write_uint32(msg, p, rlength);
      p += 4;
      lsh_string_write(msg, p, rlength, lsh_string_data(r));
      p += rlength;
    }
  assert (p == lsh_string_length(msg));
  
  return msg;
}

/* Called when we receive a USERAUTH_FAILURE message. It will
 * typically either try again, or raise EXC_USERAUTH. */

/* GABA:
   (class
     (name client_userauth_failure)
     (vars
       ; again is true if the server says that
       ; the current method is still useful.
       (failure method void "int again")))
*/

#define CLIENT_USERAUTH_FAILURE(f, c) ((f)->failure((f), (c)))

/* Almost like a command, but returns a failure handler. */
/* GABA:
   (class
     (name client_userauth_method)
     (vars
       (type . int)
       (login method (object client_userauth_failure)
                     "struct client_userauth *u"
                     "struct ssh_connection *c"
		     "struct exception_handler *e")))
*/

#define CLIENT_USERAUTH_LOGIN(m, u, c, e) \
  ((m)->login((m), (u), (c), (e)))

/* Takes a connection as argument, and attempts to login. It does this
 * by trying each METHOD in turn. As soon as one succeeds, the
 * connection is returned. When a method fails (raising a EXC_USERAUTH
 * exception), we try the next method. If all methods fail, we raise
 * EXC_USERAUTH. */

/* GABA:
   (class
     (name client_userauth)
     (super command)
     (vars
       ; The name should already be converted to utf8
       (username string)            ; Remote user name to authenticate as.
       (service_name . int)         ; Service we want to access.
       (methods object object_list) ; Authentication methods, in order.
       ))
*/

/* GABA:
   (class
     (name client_userauth_state)
     (vars
       (userauth object client_userauth)
       (connection object ssh_connection)
       (failure object client_userauth_failure)

       ; Current method
       (current . unsigned)

       ; The next method that the server has indicated is useful.
       (next . unsigned)))
*/

static struct client_userauth_state *
make_client_userauth_state(struct client_userauth *userauth,
			   struct ssh_connection *connection)
{
  NEW(client_userauth_state, self);

  trace("client_userauth.c: make_client_userauth_state\n");
  self->connection = connection;
  self->userauth = userauth;
  self->failure = NULL;
  self->current = 0;
  self->next = 1;
  
  return self;
}

/* GABA:
   (class
     (name userauth_packet_handler)
     (super packet_handler)
     (vars
       (state object client_userauth_state)))
*/


/* GABA:
   (class
     (name userauth_success_handler)
     (super packet_handler)
     (vars
       (c object command_continuation)))
*/

static void
do_userauth_success(struct packet_handler *c,
		    struct ssh_connection *connection,
		    struct lsh_string *packet)
{
  CAST(userauth_success_handler, self, c);
  struct simple_buffer buffer;

  unsigned msg_number;

  trace("client_userauth.c: do_userauth_success\n");
  
  simple_buffer_init(&buffer, STRING_LD(packet));

  if (parse_uint8(&buffer, &msg_number)
      && (msg_number == SSH_MSG_USERAUTH_SUCCESS)
      && parse_eod(&buffer))
    {
      unsigned i;
      
      verbose("User authentication successful.\n");

      for (i = SSH_FIRST_USERAUTH_GENERIC; i < SSH_FIRST_CONNECTION_GENERIC; i++) 
	connection->dispatch[i] = &connection_fail_handler;
      
      COMMAND_RETURN(self->c, connection);
    }
  else
    PROTOCOL_ERROR(connection->e, "Invalid USERAUTH_SUCCESS message");
}

static struct packet_handler *
make_success_handler(struct command_continuation *c)
{
  NEW(userauth_success_handler, self);

  self->super.handler = do_userauth_success;
  self->c = c;

  return &self->super;
}


/* GABA:
   (class
     (name failure_handler)
     (super userauth_packet_handler)
     (vars
       (e object exception_handler)))
*/


/* Arbitrary limit on list length */
#define USERAUTH_MAX_METHODS 47

static int
userauth_method_is_useful(struct client_userauth *userauth,
			  struct int_list *advertised,
			  unsigned n)
{
  CAST_SUBTYPE(client_userauth_method, method,
	       LIST(userauth->methods)[n]);

  unsigned i;

#if 0
  debug("userauth_method_is_useful, advertised:\n");
  for(i = 0; i < LIST_LENGTH(advertised); i++)
    debug("  %a\n", LIST(advertised)[i]);

  debug("  method: type = %a, class = %t\n", method->type, method);
#endif
  
  for(i = 0; i < LIST_LENGTH(advertised); i++)
    if (LIST(advertised)[i] == method->type)
      return 1;

  return 0;
}

static void
do_userauth_failure(struct packet_handler *c,
		    struct ssh_connection *connection,
		    struct lsh_string *packet)
{
  CAST(userauth_packet_handler, self, c);
  struct simple_buffer buffer;

  unsigned msg_number;
  struct int_list *methods = NULL;
  int partial_success;

  trace("client_userauth.c: do_userauth_failure\n");
  assert(self->state->current < self->state->next);
  
  simple_buffer_init(&buffer, STRING_LD(packet));

  if (parse_uint8(&buffer, &msg_number)
      && (msg_number == SSH_MSG_USERAUTH_FAILURE)
      && ( (methods = parse_atom_list(&buffer, USERAUTH_MAX_METHODS)) )
      && parse_boolean(&buffer, &partial_success)
      && parse_eod(&buffer))
    {
      if (partial_success)
	/* Doesn't help us */
	werror("Received SSH_MSH_USERAUTH_FAILURE "
	       "indicating partial success.\n");

      while ( (self->state->next < LIST_LENGTH(self->state->userauth->methods))
	      && !userauth_method_is_useful(self->state->userauth,
					    methods, self->state->next))
	self->state->next++;

      CLIENT_USERAUTH_FAILURE(self->state->failure,
			      userauth_method_is_useful(self->state->userauth,
							methods, self->state->current));
    }
  else
    PROTOCOL_ERROR(connection->e, "Invalid USERAUTH_FAILURE message.");
  
  KILL(methods);	
}

static struct packet_handler *
make_failure_handler(struct client_userauth_state *state)
{
  NEW(userauth_packet_handler, self);

  trace("client_userauth.c: make_failure_handler\n");

  self->super.handler = do_userauth_failure;
  self->state = state;

  return &self->super;
}

static void
do_userauth_banner(struct packet_handler *self UNUSED,
		   struct ssh_connection *connection UNUSED,
		   struct lsh_string *packet)
{
  struct simple_buffer buffer;

  unsigned msg_number;
  uint32_t length;
  const uint8_t *msg;

  uint32_t language_length;
  const uint8_t *language;
  
  simple_buffer_init(&buffer, STRING_LD(packet));

  if (parse_uint8(&buffer, &msg_number)
      && (msg_number == SSH_MSG_USERAUTH_BANNER)
      && parse_string(&buffer, &length, &msg)
      && parse_string(&buffer, &language_length, &language)
      && parse_eod(&buffer))
    {
      /* Ignore language tag */
      werror("%ups", length, msg);
    }
  else
    PROTOCOL_ERROR(connection->e, "Invalid USERAUTH_SUCCESS message");
}

static struct packet_handler userauth_banner_handler =
{ STATIC_HEADER, do_userauth_banner };


/* GABA:
   (class
     (name exc_client_userauth)
     (super exception_handler)
     (vars
       (state object client_userauth_state)))
*/

/* Skip to the next useful userauth method, or reraise the exception
 * if there are no more methods to try. */

static void
do_exc_client_userauth(struct exception_handler *s,
		       const struct exception *e)
{
  CAST(exc_client_userauth, self, s);

  trace("client_userauth.c: do_exc_client_userauth\n");
  assert(self->state->current < self->state->next);

  if ( (e->type == EXC_USERAUTH)
       && (self->state->next < LIST_LENGTH(self->state->userauth->methods)))
    {
      self->state->current = self->state->next++;
      {      
	CAST_SUBTYPE(client_userauth_method, method,
		     LIST(self->state->userauth->methods)[self->state->current]);
	
	self->state->failure
	  = CLIENT_USERAUTH_LOGIN(method, self->state->userauth,
				  self->state->connection, s);
      }
    }
  else
    EXCEPTION_RAISE(s->parent, e);
}

static struct exception_handler *
make_exc_client_userauth(struct client_userauth_state *state,
			 struct exception_handler *parent,
			 const char *context)
{
  NEW(exc_client_userauth, self);

  trace("client_userauth.c: make_exc_client_userauth\n");

  self->super.parent = parent;
  self->super.raise = do_exc_client_userauth;
  self->super.context = context;

  self->state = state;

  return &self->super;
}

/* GABA:
   (class
     (name exc_userauth_disconnect)
     (super exception_handler)
     (vars
       (connection object ssh_connection)))
*/

/* Disconnects the connection if user authentication fails. */
static void
do_exc_userauth_disconnect(struct exception_handler *s,
			   const struct exception *e)
{
  CAST(exc_userauth_disconnect, self, s);
  
  if (e->type == EXC_USERAUTH)
    {
      werror("User authentication failed.\n");
      EXCEPTION_RAISE(self->connection->e,
		      make_protocol_exception(SSH_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE,
					      NULL));
      /* Also raise the exception with the parent exception
	 handler. */
    }

  EXCEPTION_RAISE(s->parent, e);
}

static struct exception_handler *
make_exc_userauth_disconnect(struct ssh_connection *connection,
			     struct exception_handler *parent,
			     const char *context)
{
  NEW(exc_userauth_disconnect, self);

  trace("client_userauth.c: make_exc_userauth_disconnect\n");

  self->super.parent = parent;
  self->super.raise = do_exc_userauth_disconnect;
  self->super.context = context;

  self->connection = connection;
  return &self->super;
}

static void
do_client_userauth(struct command *s,
		   struct lsh_object *x,
		   struct command_continuation *c,
		   struct exception_handler *e)
{
  CAST(client_userauth, self, s);
  CAST(ssh_connection, connection, x);
  
  struct client_userauth_state *state
    = make_client_userauth_state(self, connection);

  trace("client_userauth.c: do_client_userauth\n");

  connection->dispatch[SSH_MSG_USERAUTH_SUCCESS]
    = make_success_handler(c);
  connection->dispatch[SSH_MSG_USERAUTH_FAILURE]
    = make_failure_handler(state);
  connection->dispatch[SSH_MSG_USERAUTH_BANNER]
    = &userauth_banner_handler;

  assert(LIST_LENGTH(self->methods));
  {
    CAST_SUBTYPE(client_userauth_method, method,
		 LIST(self->methods)[0]);
    state->failure
      = CLIENT_USERAUTH_LOGIN(method, self,
			      connection,
			      make_exc_client_userauth
			      (state,
			       make_exc_userauth_disconnect
			       (connection, e, HANDLER_CONTEXT),
			       HANDLER_CONTEXT));
  }
}


struct command *
make_client_userauth(struct lsh_string *username,
		     int service_name,
		     struct object_list *methods)
{
  NEW(client_userauth, self);

  self->super.call = do_client_userauth;
  self->username = local_to_utf8(username, 1);
  self->service_name = service_name;
  self->methods = methods;

  return &self->super;
}


/* None authentication */
/* GABA:
   (class
     (name client_none_state)
     (super client_userauth_failure)
     (vars
       (e object exception_handler)))
*/

static void
do_none_failure(struct client_userauth_failure *s, int again)
{
  CAST(client_none_state, self, s);
  
  static const struct exception need_auth =
    STATIC_EXCEPTION(EXC_USERAUTH, "Need real authentication.");

  trace("client_userauth.c: do_none_failure\n");

  if (again)
    werror("Odd. Server says we should try the `none'authentication method again.\n");

  EXCEPTION_RAISE(self->e, &need_auth);
}

static struct client_userauth_failure *
make_client_none_state(struct exception_handler *e)
{
  NEW(client_none_state, self);

  trace("client_userauth.c: make_client_none_state\n");

  self->super.failure = do_none_failure;
  self->e = e;

  return &self->super;
}

static struct client_userauth_failure *
do_none_login(struct client_userauth_method *s UNUSED,
              struct client_userauth *userauth,
              struct ssh_connection *connection,
              struct exception_handler *e)
{
  verbose("Requesting authentication using the `none' method.\n");
  
  connection_send(connection,
		  format_userauth_none(userauth->username,
				       userauth->service_name));
  
  return make_client_none_state(e);
}

static struct client_userauth_method
client_userauth_none =
  { STATIC_HEADER, ATOM_NONE, do_none_login };

struct client_userauth_method *
make_client_none_auth(void)
{
  return &client_userauth_none;
}


/* Password authentication */

#define MAX_PASSWD 100

/* GABA:
   (class
     (name client_password_state)
     (super client_userauth_failure)
     (vars
       (userauth object client_userauth)
       (tty object interact)
       ; Have we tried the empty password already?
       (tried_empty_passwd . int)
       (connection object ssh_connection)
       (e object exception_handler)))
*/

static void
send_password(struct client_password_state *state)
{
  struct lsh_string *passwd
    = INTERACT_READ_PASSWORD(state->tty, MAX_PASSWD,
			     ssh_format("Password for %lS: ",
					state->userauth->username));

  if (passwd)
    {
      /* Password empty? */
      if (!lsh_string_length(passwd))
	{
	  /* NOTE: At least on some systems, the getpass function
	   * sets the tty to raw mode, disabling ^C, ^D and the like.
	   *
	   * To be a little friendlier, we stop asking if the users
	   * gives us the empty password twice.
	   */
	  state->tried_empty_passwd++;
	}

      verbose("Requesting authentication using the `password' method.\n");
      
      connection_send(state->connection,
		      format_userauth_password(state->userauth->username,
					       state->userauth->service_name,
					       local_to_utf8(passwd, 1),
					       1));
    }
  else
    {
      /* If the user aborts the password dialogue (by pressing ^D at
       * the terminal, or by closing a password popup window,
       * whatever), read_password should return NULL, and we should
       * skip to the next authentication method. */
      static const struct exception no_passwd =
	STATIC_EXCEPTION(EXC_USERAUTH, "No password supplied.");

      EXCEPTION_RAISE(state->e, &no_passwd);
    }
}


static void
do_password_failure(struct client_userauth_failure *s, int again)
{
  CAST(client_password_state, self, s);

  trace("client_userauth.c: do_password_failure\n");

  if (again)
    {
      if (self->tried_empty_passwd >= 2)
	{
	  static const struct exception no_passwd =
	    STATIC_EXCEPTION(EXC_USERAUTH, "No password supplied.");
	  
	  EXCEPTION_RAISE(self->e, &no_passwd);
	}
      else
	send_password(self);
    }
  else
    {
      static const struct exception passwd_not_useful =
	STATIC_EXCEPTION(EXC_USERAUTH, "password authentication not useful.");

      EXCEPTION_RAISE(self->e, &passwd_not_useful);
    }
}

static struct client_password_state *
make_client_password_state(struct client_userauth *userauth,
			   struct interact *tty,
			   struct ssh_connection *connection,
			   struct exception_handler *e)
{
  NEW(client_password_state, self);

  trace("client_userauth.c: make_client_password_state\n");

  self->super.failure = do_password_failure;
  self->userauth = userauth;
  self->tty = tty;
  self->tried_empty_passwd = 0;
  self->connection = connection;
  self->e = e;

  return self;
}

/* Used for both "password" and "keyboard-interactive" */
/* GABA:
   (class
     (name client_userauth_interactive_method)
     (super client_userauth_method)
     (vars
       (tty object interact)))
*/


static struct client_userauth_failure *
do_password_login(struct client_userauth_method *s,
		  struct client_userauth *userauth,
		  struct ssh_connection *connection,
		  struct exception_handler *e)
{
  CAST(client_userauth_interactive_method, self, s);
  
  struct client_password_state *state
    = make_client_password_state(userauth, self->tty,
				 connection, e);

  trace("client_userauth.c: do_password_login\n");

  send_password(state);
  
  return &state->super;
}

struct client_userauth_method *
make_client_password_auth(struct interact *tty)
{
  NEW(client_userauth_interactive_method, self);

  self->super.type = ATOM_PASSWORD;
  self->super.login = do_password_login;
  self->tty = tty;
  
  return &self->super;
}


/* Publickey authentication. */

/* We first send a set of publickey authentication requests for the
 * available keys. This is analogous to unlocking a door by first
 * examining the keys on one's keyring to see if any of them can be
 * inserted into the lock. '
 *
 * FIXME: At this stage it is preferable to use spki hashed public
 * keys rather than the public keys themselves.
 *
 * Next we wait for SSH_MSH_USERAUTH_FAILURE or SSH_MSG_USERAUTH_PK_OK
 * messages. If any of the keys is recognized, we compute a signature
 * and send it to the server (analogous to inserting the key into the
 * lock and trying to turn it around). */

/* GABA:
   (class
     (name client_publickey_method)
     (super client_userauth_method)
     (vars
       (keys object object_list)))
*/

/* GABA:
   (class
     (name client_publickey_state)
     (super client_userauth_failure)
     (vars
       (userauth object client_userauth)
       (connection object ssh_connection)
       (keys object object_list)
       ; Number of keys for which we have received either
       ; a USERAUTH_FAILURE or USERAUTH_PK_OK message.
       (done . uint32_t)
       ; Number of keys for which we have computed and sent a signature,
       ; and not yet received any failure.
       (pending . uint32_t)
       (e object exception_handler)))
*/

static void
client_publickey_next(struct client_publickey_state *state)
{
  static const struct exception publickey_auth_failed =
    STATIC_EXCEPTION(EXC_USERAUTH, "Public key userauth failed.");

  if (state->done < LIST_LENGTH(state->keys))
    {
      /* We received a response on a query request. */
      
      state->done++;
      if (state->done < LIST_LENGTH(state->keys))
	/* We are still waiting for SSH_MSG_USERAUTH_FAILURE or
	 * SSH_MSG_USERAUTH_PK_OK */
	return;

      /* We have got a resposne on the final query request. */
      state->connection->dispatch[SSH_MSG_USERAUTH_PK_OK]
	= &connection_fail_handler;
    }
  else
    {
      /* We received a response (actually, a failure) on a request
       * that included a signature. */
      assert(state->pending);
      state->pending--;
    }

  if (!state->pending)
    /* All failed. */
    EXCEPTION_RAISE(state->e, &publickey_auth_failed);
}

static void
do_publickey_failure(struct client_userauth_failure *s, int again UNUSED)
{
  CAST(client_publickey_state, self, s);

  /* NOTE: We have several pending requests, so we have to get all the
   * replies before skipping to the next method. That's why the again
   * argument must be ignored. */
  client_publickey_next(self);
}
  
static struct client_publickey_state *
make_client_publickey_state(struct client_userauth *userauth,
			    struct ssh_connection *connection,
			    struct object_list *keys,
			    struct exception_handler *e)
{
  NEW(client_publickey_state, self);
  self->super.failure = do_publickey_failure;
  self->userauth = userauth;
  self->connection = connection;
  self->keys = keys;
  self->done = 0;
  self->pending = 0;
  self->e = e;

  return self;
}

/* GABA:
   (class
     (name userauth_pk_ok_handler)
     (super packet_handler)
     (vars
       (state object client_publickey_state))) */
  
static void 
do_userauth_pk_ok(struct packet_handler *s,
		  struct ssh_connection *connection,
		  struct lsh_string *packet)
{
  CAST(userauth_pk_ok_handler, self, s);

  struct simple_buffer buffer;

  unsigned msg_number;
  int algorithm;
  uint32_t keyblob_length;
  const uint8_t *keyblob;

  simple_buffer_init(&buffer, STRING_LD(packet));

  if (parse_uint8(&buffer, &msg_number)
      && (msg_number == SSH_MSG_USERAUTH_PK_OK)
      && parse_atom(&buffer, &algorithm)
      && parse_string(&buffer, &keyblob_length, &keyblob)
      && parse_eod(&buffer))
    {
      CAST(keypair, key, LIST(self->state->keys)[self->state->done]);
      verbose("SSH_MSG_USERAUTH_PK_OK received\n");
	  
      if ( (key->type == algorithm)
	   && lsh_string_eq_l(key->public, keyblob_length, keyblob) )
	{
	  struct lsh_string *request;
	  struct lsh_string *signed_data;

	  verbose("Sending `publickey' signature.\n");
  
	  request = format_userauth_publickey(self->state->userauth->username,
					      self->state->userauth->service_name,
					      key->type,
					      key->public);

	  signed_data = ssh_format("%S%lS", connection->session_id, request);
	  request = ssh_format("%flS%fS", 
			       request, 
			       SIGN(key->private, key->type,
				    lsh_string_length(signed_data),
				    lsh_string_data(signed_data)));
	  lsh_string_free(signed_data);
	  connection_send(connection, request);
	  self->state->pending++;
	}
      else
	werror("client_userauth.c: Unexpected key in USERAUTH_PK_OK message.\n");

      client_publickey_next(self->state);
    }
  else
    PROTOCOL_ERROR(self->state->e, "Invalid USERAUTH_PK_OK message");
}

static struct packet_handler *
make_pk_ok_handler(struct client_publickey_state *state)
{
  NEW(userauth_pk_ok_handler, self);

  self->super.handler = do_userauth_pk_ok;
  self->state = state;

  return &self->super;
}

static struct client_userauth_failure *
do_publickey_login(struct client_userauth_method *s,
		   struct client_userauth *userauth,
		   struct ssh_connection *connection,
		   struct exception_handler *e)
{
  CAST(client_publickey_method, self, s);

  assert(LIST_LENGTH(self->keys));
  
    
  {
    struct client_publickey_state *state =
      make_client_publickey_state(userauth,
				  connection,
				  self->keys,
				  e);
    unsigned i;
      
    for (i = 0; i < LIST_LENGTH(self->keys); i++)
      {
	CAST(keypair, key, LIST(self->keys)[i]);

	verbose("Requesting authentication using the `publickey' method.\n");
	
	connection_send(connection, 
			format_userauth_publickey_query
			  (userauth->username,
			   userauth->service_name,
			   key->type, key->public));
      }
    connection->dispatch[SSH_MSG_USERAUTH_PK_OK] = make_pk_ok_handler(state);
    return &state->super;
  }
}

struct client_userauth_method *
make_client_publickey_auth(struct object_list *keys)
{
  NEW(client_publickey_method, self);

  self->super.type = ATOM_PUBLICKEY;
  self->super.login = do_publickey_login;
  self->keys = keys;
  
  return &self->super;
}


/* "keyboard-interact" authentication
 *
 * Problems with the spec (draft-ietf-secsh-auth-kbdinteract-06.txt):
 *
 *  * The deprecated language tags.
 *
 *  * Use of the type int, which is undefined (should be uint32_t).
 *
 *  * The supplied name. What is it supposed to mean if that differs
 *    from the name in the USERAUTH_REQUEST?
 *
 * *  The submethods list use utf8. Should be ordinary ssh-names, in
 *    ascii and with the @domain convention for non-standard methods.
 */

/* GABA:
   (class
     (name client_kbdinteract_state)
     (super client_userauth_failure)
     (vars
       (tty object interact)
       (connection object ssh_connection)
       (e object exception_handler)))
*/

/* GABA:
   (class
     (name userauth_info_request_handler)
     (super packet_handler)
     (vars 
       (state object client_kbdinteract_state)))
*/

#define KBDINTERACT_MAX_PROMPTS 17
#define KBDINTERACT_MAX_LENGTH 200

/* FIXME: Doesn't get control character filtering right. */

static void 
do_userauth_info_request(struct packet_handler *s,
			 struct ssh_connection *connection,
			 struct lsh_string *packet)
{
  CAST(userauth_info_request_handler, self, s);

  struct simple_buffer buffer;

  unsigned msg_number;
  const uint8_t *name;
  uint32_t name_length;

  const uint8_t *instruction;
  uint32_t instruction_length;
  /* Deprecated and ignored */
  const uint8_t *language;
  uint32_t language_length;

  /* Typed as "int" in the spec. Hope that means uint32_t? */  
  uint32_t nprompt; 
  
  simple_buffer_init(&buffer, STRING_LD(packet));
  if (parse_uint8(&buffer, &msg_number)
      && (msg_number == SSH_MSG_USERAUTH_INFO_REQUEST)
      && parse_string(&buffer, &name_length, &name)
      && parse_string(&buffer, &instruction_length, &instruction)
      && parse_string(&buffer, &language_length, &language)
      && parse_uint32(&buffer, &nprompt))
    {
      struct interact_dialog *dialog;
      unsigned i;
      
      if (nprompt > KBDINTERACT_MAX_PROMPTS
	  || name_length > KBDINTERACT_MAX_LENGTH
	  || instruction_length > 10*KBDINTERACT_MAX_LENGTH)
	{
	  static const struct exception bad_info_request =
	    STATIC_EXCEPTION(EXC_USERAUTH, "Too large USERAUTH_INFO_RQUEST");
	beyond_limit:

	  EXCEPTION_RAISE(self->state->e, &bad_info_request);
	  return;
	}

      dialog = make_interact_dialog(nprompt);

      for (i = 0; i < nprompt; i++)
	{
	  const uint8_t *prompt;
	  uint32_t prompt_length;
	  struct lsh_string *s;

	  if (! (parse_string(&buffer, &prompt_length, &prompt)
		 && parse_boolean(&buffer, &dialog->echo[i])))
	    {
	      KILL(dialog);
	      goto error;
	    }

	  if (prompt_length > KBDINTERACT_MAX_LENGTH)
	    {
	      KILL(dialog);
	      goto beyond_limit;
	    }
	  s = low_utf8_to_local(prompt_length, prompt,
				utf8_replace | utf8_paranoid);
	  if (!s)
	    goto error;
	  
	  dialog->prompt[i] = s;
	}
      
      dialog->instruction
	= low_utf8_to_local(instruction_length, instruction,
			    utf8_replace | utf8_paranoid);
      
      if (!dialog->instruction)
	goto error;

      if (name_length > 0)
	{
	  /* Prepend to instruction */
	  struct lsh_string *s;
      
	  s = low_utf8_to_local(name_length, name,
				utf8_replace | utf8_paranoid);
	  if (!s)
	    goto error;

	  dialog->instruction = ssh_format("%lfS\n\n%lfS\n",
					   s, dialog->instruction);
	}
      else
	dialog->instruction = ssh_format("%lfS\n", dialog->instruction);

      if (!INTERACT_DIALOG(self->state->tty, dialog))
	{
	  static const struct exception bad_info_request =
	    STATIC_EXCEPTION(EXC_USERAUTH, "No user response");

	  EXCEPTION_RAISE(self->state->e, &bad_info_request);
	  return;
	}

      connection_send(connection, format_userauth_info_response(dialog));
      KILL(dialog);
      return;
    }
  else
    {
    error:
      PROTOCOL_ERROR(self->state->e, "Invalid USERAUTH_INFO_REQUEST message");
    }
}

static struct packet_handler *
make_userauth_info_request_handler(struct client_kbdinteract_state *state)
{
  NEW(userauth_info_request_handler, self);
  self->super.handler = do_userauth_info_request;
  self->state = state;

  return &self->super;
}

static void
do_kbdinteract_failure(struct client_userauth_failure *s, int again UNUSED)
{
  CAST(client_kbdinteract_state, self, s);

  /* FIXME: We never try again. How do we know if the user wants to
     move on? */
  static const struct exception kbdinteract_fail =
    STATIC_EXCEPTION(EXC_USERAUTH, "keyboard-interact authentication failed.");

  EXCEPTION_RAISE(self->e, &kbdinteract_fail);  
}

static struct client_kbdinteract_state *
make_client_kbdinteract_state(struct interact *tty,
			      struct ssh_connection *connection,
			      struct exception_handler *e)
{
  NEW(client_kbdinteract_state, self);
  self->super.failure = do_kbdinteract_failure;
  self->tty = tty;
  self->connection = connection;
  self->e = e;

  return self;
}

static struct client_userauth_failure *
do_kbdinteract_login(struct client_userauth_method *s,
		     struct client_userauth *userauth,
		     struct ssh_connection *connection,
		     struct exception_handler *e)
{
  CAST(client_userauth_interactive_method, self, s);
  struct client_kbdinteract_state *state
    = make_client_kbdinteract_state(self->tty, connection, e);

  verbose("Requesting authentication using the `keyboard-interactive' method.\n");
  
  connection_send(connection,
		  format_userauth_kbdinteract(userauth->username, userauth->service_name));

  connection->dispatch[SSH_MSG_USERAUTH_INFO_REQUEST]
    = make_userauth_info_request_handler(state);

  return &state->super;
}

struct client_userauth_method *
make_client_kbdinteract_auth(struct interact *tty)
{
  NEW(client_userauth_interactive_method, self);

  self->super.type = ATOM_KEYBOARD_INTERACTIVE;
  self->super.login = do_kbdinteract_login;
  self->tty = tty;
  
  return &self->super;
}
