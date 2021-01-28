/*
CLASS:client_userauth_failure:
*/
#ifndef GABA_DEFINE
struct client_userauth_failure
{
  struct lsh_object super;
  void (*(failure))(struct client_userauth_failure *self,int again);
};
extern struct lsh_class client_userauth_failure_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class client_userauth_failure_class =
{
  STATIC_HEADER,
  NULL,
  "client_userauth_failure",
  sizeof(struct client_userauth_failure),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:client_userauth_method:
*/
#ifndef GABA_DEFINE
struct client_userauth_method
{
  struct lsh_object super;
  int type;
  struct client_userauth_failure *(*(login))(struct client_userauth_method *self,struct client_userauth *u,struct ssh_connection *c,struct exception_handler *e);
};
extern struct lsh_class client_userauth_method_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class client_userauth_method_class =
{
  STATIC_HEADER,
  NULL,
  "client_userauth_method",
  sizeof(struct client_userauth_method),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:client_userauth:command
*/
#ifndef GABA_DEFINE
struct client_userauth
{
  struct command super;
  struct lsh_string *username;
  int service_name;
  struct object_list *methods;
};
extern struct lsh_class client_userauth_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_client_userauth_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct client_userauth *i = (struct client_userauth *) o;
  mark((struct lsh_object *) i->methods);
}
static void
do_client_userauth_free(struct lsh_object *o)
{
  struct client_userauth *i = (struct client_userauth *) o;
  lsh_string_free(i->username);
}
struct lsh_class client_userauth_class =
{
  STATIC_HEADER,
  &(command_class),
  "client_userauth",
  sizeof(struct client_userauth),
  do_client_userauth_mark,
  do_client_userauth_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:client_userauth_state:
*/
#ifndef GABA_DEFINE
struct client_userauth_state
{
  struct lsh_object super;
  struct client_userauth *userauth;
  struct ssh_connection *connection;
  struct client_userauth_failure *failure;
  unsigned current;
  unsigned next;
};
extern struct lsh_class client_userauth_state_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_client_userauth_state_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct client_userauth_state *i = (struct client_userauth_state *) o;
  mark((struct lsh_object *) i->userauth);
  mark((struct lsh_object *) i->connection);
  mark((struct lsh_object *) i->failure);
}
struct lsh_class client_userauth_state_class =
{
  STATIC_HEADER,
  NULL,
  "client_userauth_state",
  sizeof(struct client_userauth_state),
  do_client_userauth_state_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:userauth_packet_handler:packet_handler
*/
#ifndef GABA_DEFINE
struct userauth_packet_handler
{
  struct packet_handler super;
  struct client_userauth_state *state;
};
extern struct lsh_class userauth_packet_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_userauth_packet_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct userauth_packet_handler *i = (struct userauth_packet_handler *) o;
  mark((struct lsh_object *) i->state);
}
struct lsh_class userauth_packet_handler_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "userauth_packet_handler",
  sizeof(struct userauth_packet_handler),
  do_userauth_packet_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:userauth_success_handler:packet_handler
*/
#ifndef GABA_DEFINE
struct userauth_success_handler
{
  struct packet_handler super;
  struct command_continuation *c;
};
extern struct lsh_class userauth_success_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_userauth_success_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct userauth_success_handler *i = (struct userauth_success_handler *) o;
  mark((struct lsh_object *) i->c);
}
struct lsh_class userauth_success_handler_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "userauth_success_handler",
  sizeof(struct userauth_success_handler),
  do_userauth_success_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:failure_handler:userauth_packet_handler
*/
#ifndef GABA_DEFINE
struct failure_handler
{
  struct userauth_packet_handler super;
  struct exception_handler *e;
};
extern struct lsh_class failure_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_failure_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct failure_handler *i = (struct failure_handler *) o;
  mark((struct lsh_object *) i->e);
}
struct lsh_class failure_handler_class =
{
  STATIC_HEADER,
  &(userauth_packet_handler_class),
  "failure_handler",
  sizeof(struct failure_handler),
  do_failure_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:exc_client_userauth:exception_handler
*/
#ifndef GABA_DEFINE
struct exc_client_userauth
{
  struct exception_handler super;
  struct client_userauth_state *state;
};
extern struct lsh_class exc_client_userauth_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_exc_client_userauth_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct exc_client_userauth *i = (struct exc_client_userauth *) o;
  mark((struct lsh_object *) i->state);
}
struct lsh_class exc_client_userauth_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "exc_client_userauth",
  sizeof(struct exc_client_userauth),
  do_exc_client_userauth_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:exc_userauth_disconnect:exception_handler
*/
#ifndef GABA_DEFINE
struct exc_userauth_disconnect
{
  struct exception_handler super;
  struct ssh_connection *connection;
};
extern struct lsh_class exc_userauth_disconnect_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_exc_userauth_disconnect_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct exc_userauth_disconnect *i = (struct exc_userauth_disconnect *) o;
  mark((struct lsh_object *) i->connection);
}
struct lsh_class exc_userauth_disconnect_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "exc_userauth_disconnect",
  sizeof(struct exc_userauth_disconnect),
  do_exc_userauth_disconnect_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:client_none_state:client_userauth_failure
*/
#ifndef GABA_DEFINE
struct client_none_state
{
  struct client_userauth_failure super;
  struct exception_handler *e;
};
extern struct lsh_class client_none_state_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_client_none_state_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct client_none_state *i = (struct client_none_state *) o;
  mark((struct lsh_object *) i->e);
}
struct lsh_class client_none_state_class =
{
  STATIC_HEADER,
  &(client_userauth_failure_class),
  "client_none_state",
  sizeof(struct client_none_state),
  do_client_none_state_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:client_password_state:client_userauth_failure
*/
#ifndef GABA_DEFINE
struct client_password_state
{
  struct client_userauth_failure super;
  struct client_userauth *userauth;
  struct interact *tty;
  int tried_empty_passwd;
  struct ssh_connection *connection;
  struct exception_handler *e;
};
extern struct lsh_class client_password_state_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_client_password_state_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct client_password_state *i = (struct client_password_state *) o;
  mark((struct lsh_object *) i->userauth);
  mark((struct lsh_object *) i->tty);
  mark((struct lsh_object *) i->connection);
  mark((struct lsh_object *) i->e);
}
struct lsh_class client_password_state_class =
{
  STATIC_HEADER,
  &(client_userauth_failure_class),
  "client_password_state",
  sizeof(struct client_password_state),
  do_client_password_state_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:client_userauth_interactive_method:client_userauth_method
*/
#ifndef GABA_DEFINE
struct client_userauth_interactive_method
{
  struct client_userauth_method super;
  struct interact *tty;
};
extern struct lsh_class client_userauth_interactive_method_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_client_userauth_interactive_method_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct client_userauth_interactive_method *i = (struct client_userauth_interactive_method *) o;
  mark((struct lsh_object *) i->tty);
}
struct lsh_class client_userauth_interactive_method_class =
{
  STATIC_HEADER,
  &(client_userauth_method_class),
  "client_userauth_interactive_method",
  sizeof(struct client_userauth_interactive_method),
  do_client_userauth_interactive_method_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:client_publickey_method:client_userauth_method
*/
#ifndef GABA_DEFINE
struct client_publickey_method
{
  struct client_userauth_method super;
  struct object_list *keys;
};
extern struct lsh_class client_publickey_method_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_client_publickey_method_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct client_publickey_method *i = (struct client_publickey_method *) o;
  mark((struct lsh_object *) i->keys);
}
struct lsh_class client_publickey_method_class =
{
  STATIC_HEADER,
  &(client_userauth_method_class),
  "client_publickey_method",
  sizeof(struct client_publickey_method),
  do_client_publickey_method_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:client_publickey_state:client_userauth_failure
*/
#ifndef GABA_DEFINE
struct client_publickey_state
{
  struct client_userauth_failure super;
  struct client_userauth *userauth;
  struct ssh_connection *connection;
  struct object_list *keys;
  uint32_t done;
  uint32_t pending;
  struct exception_handler *e;
};
extern struct lsh_class client_publickey_state_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_client_publickey_state_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct client_publickey_state *i = (struct client_publickey_state *) o;
  mark((struct lsh_object *) i->userauth);
  mark((struct lsh_object *) i->connection);
  mark((struct lsh_object *) i->keys);
  mark((struct lsh_object *) i->e);
}
struct lsh_class client_publickey_state_class =
{
  STATIC_HEADER,
  &(client_userauth_failure_class),
  "client_publickey_state",
  sizeof(struct client_publickey_state),
  do_client_publickey_state_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:userauth_pk_ok_handler:packet_handler
*/
#ifndef GABA_DEFINE
struct userauth_pk_ok_handler
{
  struct packet_handler super;
  struct client_publickey_state *state;
};
extern struct lsh_class userauth_pk_ok_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_userauth_pk_ok_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct userauth_pk_ok_handler *i = (struct userauth_pk_ok_handler *) o;
  mark((struct lsh_object *) i->state);
}
struct lsh_class userauth_pk_ok_handler_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "userauth_pk_ok_handler",
  sizeof(struct userauth_pk_ok_handler),
  do_userauth_pk_ok_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:client_kbdinteract_state:client_userauth_failure
*/
#ifndef GABA_DEFINE
struct client_kbdinteract_state
{
  struct client_userauth_failure super;
  struct interact *tty;
  struct ssh_connection *connection;
  struct exception_handler *e;
};
extern struct lsh_class client_kbdinteract_state_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_client_kbdinteract_state_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct client_kbdinteract_state *i = (struct client_kbdinteract_state *) o;
  mark((struct lsh_object *) i->tty);
  mark((struct lsh_object *) i->connection);
  mark((struct lsh_object *) i->e);
}
struct lsh_class client_kbdinteract_state_class =
{
  STATIC_HEADER,
  &(client_userauth_failure_class),
  "client_kbdinteract_state",
  sizeof(struct client_kbdinteract_state),
  do_client_kbdinteract_state_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:userauth_info_request_handler:packet_handler
*/
#ifndef GABA_DEFINE
struct userauth_info_request_handler
{
  struct packet_handler super;
  struct client_kbdinteract_state *state;
};
extern struct lsh_class userauth_info_request_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_userauth_info_request_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct userauth_info_request_handler *i = (struct userauth_info_request_handler *) o;
  mark((struct lsh_object *) i->state);
}
struct lsh_class userauth_info_request_handler_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "userauth_info_request_handler",
  sizeof(struct userauth_info_request_handler),
  do_userauth_info_request_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

