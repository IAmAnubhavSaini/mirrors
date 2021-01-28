/*
CLASS:userauth_handler:packet_handler
*/
#ifndef GABA_DEFINE
struct userauth_handler
{
  struct packet_handler super;
  struct command_continuation *c;
  struct exception_handler *service_e;
  struct exception_handler *auth_e;
  struct alist *methods;
  struct alist *services;
};
extern struct lsh_class userauth_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_userauth_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct userauth_handler *i = (struct userauth_handler *) o;
  mark((struct lsh_object *) i->c);
  mark((struct lsh_object *) i->service_e);
  mark((struct lsh_object *) i->auth_e);
  mark((struct lsh_object *) i->methods);
  mark((struct lsh_object *) i->services);
}
struct lsh_class userauth_handler_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "userauth_handler",
  sizeof(struct userauth_handler),
  do_userauth_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:userauth_continuation:command_continuation
*/
#ifndef GABA_DEFINE
struct userauth_continuation
{
  struct command_continuation super;
  struct command_continuation *up;
  struct ssh_connection *connection;
};
extern struct lsh_class userauth_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_userauth_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct userauth_continuation *i = (struct userauth_continuation *) o;
  mark((struct lsh_object *) i->up);
  mark((struct lsh_object *) i->connection);
}
struct lsh_class userauth_continuation_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "userauth_continuation",
  sizeof(struct userauth_continuation),
  do_userauth_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:exc_userauth_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct exc_userauth_handler
{
  struct exception_handler super;
  struct ssh_connection *connection;
  struct int_list *advertised_methods;
  unsigned attempts;
};
extern struct lsh_class exc_userauth_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_exc_userauth_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct exc_userauth_handler *i = (struct exc_userauth_handler *) o;
  mark((struct lsh_object *) i->connection);
  mark((struct lsh_object *) i->advertised_methods);
}
struct lsh_class exc_userauth_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "exc_userauth_handler",
  sizeof(struct exc_userauth_handler),
  do_exc_userauth_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:userauth_none_permit:userauth
*/
#ifndef GABA_DEFINE
struct userauth_none_permit
{
  struct userauth super;
  struct lsh_user *user;
};
extern struct lsh_class userauth_none_permit_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_userauth_none_permit_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct userauth_none_permit *i = (struct userauth_none_permit *) o;
  mark((struct lsh_object *) i->user);
}
struct lsh_class userauth_none_permit_class =
{
  STATIC_HEADER,
  &(userauth_class),
  "userauth_none_permit",
  sizeof(struct userauth_none_permit),
  do_userauth_none_permit_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

