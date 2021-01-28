/*
CLASS:escape_help:escape_callback
*/
#ifndef GABA_DEFINE
struct escape_help
{
  struct escape_callback super;
  struct escape_info *info;
};
extern struct lsh_class escape_help_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_escape_help_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct escape_help *i = (struct escape_help *) o;
  mark((struct lsh_object *) i->info);
}
struct lsh_class escape_help_class =
{
  STATIC_HEADER,
  &(escape_callback_class),
  "escape_help",
  sizeof(struct escape_help),
  do_escape_help_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:escape_handler:abstract_write_pipe
*/
#ifndef GABA_DEFINE
struct escape_handler
{
  struct abstract_write_pipe super;
  struct escape_info *info;
  enum escape_state state;
};
extern struct lsh_class escape_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_escape_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct escape_handler *i = (struct escape_handler *) o;
  mark((struct lsh_object *) i->info);
}
struct lsh_class escape_handler_class =
{
  STATIC_HEADER,
  &(abstract_write_pipe_class),
  "escape_handler",
  sizeof(struct escape_handler),
  do_escape_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

