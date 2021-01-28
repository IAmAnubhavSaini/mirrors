/*
CLASS:install_global_request_handler:command
*/
#ifndef GABA_DEFINE
struct install_global_request_handler
{
  struct command super;
  int name;
  struct global_request *handler;
};
extern struct lsh_class install_global_request_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_install_global_request_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct install_global_request_handler *i = (struct install_global_request_handler *) o;
  mark((struct lsh_object *) i->handler);
}
struct lsh_class install_global_request_handler_class =
{
  STATIC_HEADER,
  &(command_class),
  "install_global_request_handler",
  sizeof(struct install_global_request_handler),
  do_install_global_request_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:install_channel_open_handler:command
*/
#ifndef GABA_DEFINE
struct install_channel_open_handler
{
  struct command super;
  int name;
  struct channel_open *handler;
};
extern struct lsh_class install_channel_open_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_install_channel_open_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct install_channel_open_handler *i = (struct install_channel_open_handler *) o;
  mark((struct lsh_object *) i->handler);
}
struct lsh_class install_channel_open_handler_class =
{
  STATIC_HEADER,
  &(command_class),
  "install_channel_open_handler",
  sizeof(struct install_channel_open_handler),
  do_install_channel_open_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

