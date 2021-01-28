/*
CLASS:connect_continuation:command_continuation
*/
#ifndef GABA_DEFINE
struct connect_continuation
{
  struct command_continuation super;
  struct address_info *target;
  struct command_continuation *up;
};
extern struct lsh_class connect_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_connect_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct connect_continuation *i = (struct connect_continuation *) o;
  mark((struct lsh_object *) i->target);
  mark((struct lsh_object *) i->up);
}
struct lsh_class connect_continuation_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "connect_continuation",
  sizeof(struct connect_continuation),
  do_connect_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:connect_port:command
*/
#ifndef GABA_DEFINE
struct connect_port
{
  struct command super;
  struct address_info *target;
};
extern struct lsh_class connect_port_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_connect_port_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct connect_port *i = (struct connect_port *) o;
  mark((struct lsh_object *) i->target);
}
struct lsh_class connect_port_class =
{
  STATIC_HEADER,
  &(command_class),
  "connect_port",
  sizeof(struct connect_port),
  do_connect_port_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:tcp_wrapper:command
*/
#ifndef GABA_DEFINE
struct tcp_wrapper
{
  struct command super;
  struct lsh_string *name;
  struct lsh_string *msg;
};
extern struct lsh_class tcp_wrapper_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_tcp_wrapper_free(struct lsh_object *o)
{
  struct tcp_wrapper *i = (struct tcp_wrapper *) o;
  lsh_string_free(i->name);
  lsh_string_free(i->msg);
}
struct lsh_class tcp_wrapper_class =
{
  STATIC_HEADER,
  &(command_class),
  "tcp_wrapper",
  sizeof(struct tcp_wrapper),
  NULL,
  do_tcp_wrapper_free,
};
#endif /* !GABA_DECLARE */

