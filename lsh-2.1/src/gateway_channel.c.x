/*
CLASS:gateway_channel_open_command:channel_open_command
*/
#ifndef GABA_DEFINE
struct gateway_channel_open_command
{
  struct channel_open_command super;
  struct lsh_string *type;
  uint32_t rec_window_size;
  uint32_t rec_max_packet;
  struct alist *requests;
  struct lsh_string *args;
};
extern struct lsh_class gateway_channel_open_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_gateway_channel_open_command_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct gateway_channel_open_command *i = (struct gateway_channel_open_command *) o;
  mark((struct lsh_object *) i->requests);
}
static void
do_gateway_channel_open_command_free(struct lsh_object *o)
{
  struct gateway_channel_open_command *i = (struct gateway_channel_open_command *) o;
  lsh_string_free(i->type);
  lsh_string_free(i->args);
}
struct lsh_class gateway_channel_open_command_class =
{
  STATIC_HEADER,
  &(channel_open_command_class),
  "gateway_channel_open_command",
  sizeof(struct gateway_channel_open_command),
  do_gateway_channel_open_command_mark,
  do_gateway_channel_open_command_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:general_channel_request_command:channel_request_command
*/
#ifndef GABA_DEFINE
struct general_channel_request_command
{
  struct channel_request_command super;
  struct lsh_string *request;
};
extern struct lsh_class general_channel_request_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_general_channel_request_command_free(struct lsh_object *o)
{
  struct general_channel_request_command *i = (struct general_channel_request_command *) o;
  lsh_string_free(i->request);
}
struct lsh_class general_channel_request_command_class =
{
  STATIC_HEADER,
  &(channel_request_command_class),
  "general_channel_request_command",
  sizeof(struct general_channel_request_command),
  NULL,
  do_general_channel_request_command_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:general_global_request_command:global_request_command
*/
#ifndef GABA_DEFINE
struct general_global_request_command
{
  struct global_request_command super;
  struct lsh_string *request;
};
extern struct lsh_class general_global_request_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_general_global_request_command_free(struct lsh_object *o)
{
  struct general_global_request_command *i = (struct general_global_request_command *) o;
  lsh_string_free(i->request);
}
struct lsh_class general_global_request_command_class =
{
  STATIC_HEADER,
  &(global_request_command_class),
  "general_global_request_command",
  sizeof(struct general_global_request_command),
  NULL,
  do_general_global_request_command_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:gateway_channel_open_continuation:command_continuation
*/
#ifndef GABA_DEFINE
struct gateway_channel_open_continuation
{
  struct command_continuation super;
  struct command_continuation *up;
  struct channel_request *fallback;
};
extern struct lsh_class gateway_channel_open_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_gateway_channel_open_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct gateway_channel_open_continuation *i = (struct gateway_channel_open_continuation *) o;
  mark((struct lsh_object *) i->up);
  mark((struct lsh_object *) i->fallback);
}
struct lsh_class gateway_channel_open_continuation_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "gateway_channel_open_continuation",
  sizeof(struct gateway_channel_open_continuation),
  do_gateway_channel_open_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

