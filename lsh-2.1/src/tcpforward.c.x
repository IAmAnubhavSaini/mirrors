/*
CLASS:open_forwarded_tcpip_continuation:command_continuation
*/
#ifndef GABA_DEFINE
struct open_forwarded_tcpip_continuation
{
  struct command_continuation super;
  struct command_continuation *up;
  struct ssh_connection *connection;
};
extern struct lsh_class open_forwarded_tcpip_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_open_forwarded_tcpip_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct open_forwarded_tcpip_continuation *i = (struct open_forwarded_tcpip_continuation *) o;
  mark((struct lsh_object *) i->up);
  mark((struct lsh_object *) i->connection);
}
struct lsh_class open_forwarded_tcpip_continuation_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "open_forwarded_tcpip_continuation",
  sizeof(struct open_forwarded_tcpip_continuation),
  do_open_forwarded_tcpip_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:channel_open_direct_tcpip:channel_open
*/
#ifndef GABA_DEFINE
struct channel_open_direct_tcpip
{
  struct channel_open super;
  struct command *callback;
};
extern struct lsh_class channel_open_direct_tcpip_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_channel_open_direct_tcpip_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct channel_open_direct_tcpip *i = (struct channel_open_direct_tcpip *) o;
  mark((struct lsh_object *) i->callback);
}
struct lsh_class channel_open_direct_tcpip_class =
{
  STATIC_HEADER,
  &(channel_open_class),
  "channel_open_direct_tcpip",
  sizeof(struct channel_open_direct_tcpip),
  do_channel_open_direct_tcpip_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:tcpip_forward_request_continuation:command_continuation
*/
#ifndef GABA_DEFINE
struct tcpip_forward_request_continuation
{
  struct command_continuation super;
  struct local_port *forward;
  struct ssh_connection *connection;
  struct command_continuation *c;
};
extern struct lsh_class tcpip_forward_request_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_tcpip_forward_request_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct tcpip_forward_request_continuation *i = (struct tcpip_forward_request_continuation *) o;
  mark((struct lsh_object *) i->forward);
  mark((struct lsh_object *) i->connection);
  mark((struct lsh_object *) i->c);
}
struct lsh_class tcpip_forward_request_continuation_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "tcpip_forward_request_continuation",
  sizeof(struct tcpip_forward_request_continuation),
  do_tcpip_forward_request_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:tcpip_forward_request_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct tcpip_forward_request_handler
{
  struct exception_handler super;
  struct ssh_connection *connection;
  struct local_port *forward;
};
extern struct lsh_class tcpip_forward_request_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_tcpip_forward_request_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct tcpip_forward_request_handler *i = (struct tcpip_forward_request_handler *) o;
  mark((struct lsh_object *) i->connection);
  mark((struct lsh_object *) i->forward);
}
struct lsh_class tcpip_forward_request_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "tcpip_forward_request_handler",
  sizeof(struct tcpip_forward_request_handler),
  do_tcpip_forward_request_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:tcpip_forward_request:global_request
*/
#ifndef GABA_DEFINE
struct tcpip_forward_request
{
  struct global_request super;
  struct command *callback;
};
extern struct lsh_class tcpip_forward_request_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_tcpip_forward_request_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct tcpip_forward_request *i = (struct tcpip_forward_request *) o;
  mark((struct lsh_object *) i->callback);
}
struct lsh_class tcpip_forward_request_class =
{
  STATIC_HEADER,
  &(global_request_class),
  "tcpip_forward_request",
  sizeof(struct tcpip_forward_request),
  do_tcpip_forward_request_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

