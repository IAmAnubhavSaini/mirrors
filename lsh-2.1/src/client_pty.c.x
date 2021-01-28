/*
CLASS:pty_request:channel_request_command
*/
#ifndef GABA_DEFINE
struct pty_request
{
  struct channel_request_command super;
  struct interact *tty;
  struct lsh_string *term;
  struct terminal_attributes *attr;
  struct terminal_dimensions dims;
};
extern struct lsh_class pty_request_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_pty_request_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct pty_request *i = (struct pty_request *) o;
  mark((struct lsh_object *) i->tty);
  mark((struct lsh_object *) i->attr);
}
static void
do_pty_request_free(struct lsh_object *o)
{
  struct pty_request *i = (struct pty_request *) o;
  lsh_string_free(i->term);
}
struct lsh_class pty_request_class =
{
  STATIC_HEADER,
  &(channel_request_command_class),
  "pty_request",
  sizeof(struct pty_request),
  do_pty_request_mark,
  do_pty_request_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:client_tty_resource:resource
*/
#ifndef GABA_DEFINE
struct client_tty_resource
{
  struct resource super;
  struct interact *tty;
  struct terminal_attributes *attr;
};
extern struct lsh_class client_tty_resource_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_client_tty_resource_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct client_tty_resource *i = (struct client_tty_resource *) o;
  mark((struct lsh_object *) i->tty);
  mark((struct lsh_object *) i->attr);
}
struct lsh_class client_tty_resource_class =
{
  STATIC_HEADER,
  &(resource_class),
  "client_tty_resource",
  sizeof(struct client_tty_resource),
  do_client_tty_resource_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:client_winch_handler:window_change_callback
*/
#ifndef GABA_DEFINE
struct client_winch_handler
{
  struct window_change_callback super;
  struct ssh_channel *channel;
};
extern struct lsh_class client_winch_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_client_winch_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct client_winch_handler *i = (struct client_winch_handler *) o;
  mark((struct lsh_object *) i->channel);
}
struct lsh_class client_winch_handler_class =
{
  STATIC_HEADER,
  &(window_change_callback_class),
  "client_winch_handler",
  sizeof(struct client_winch_handler),
  do_client_winch_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:pty_request_continuation:command_frame
*/
#ifndef GABA_DEFINE
struct pty_request_continuation
{
  struct command_frame super;
  struct pty_request *req;
};
extern struct lsh_class pty_request_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_pty_request_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct pty_request_continuation *i = (struct pty_request_continuation *) o;
  mark((struct lsh_object *) i->req);
}
struct lsh_class pty_request_continuation_class =
{
  STATIC_HEADER,
  &(command_frame_class),
  "pty_request_continuation",
  sizeof(struct pty_request_continuation),
  do_pty_request_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

