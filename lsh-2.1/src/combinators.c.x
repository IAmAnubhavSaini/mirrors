/*
CLASS:command_S_continuation:command_frame
*/
#ifndef GABA_DEFINE
struct command_S_continuation
{
  struct command_frame super;
  struct command *g;
  struct lsh_object *x;
};
extern struct lsh_class command_S_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_command_S_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct command_S_continuation *i = (struct command_S_continuation *) o;
  mark((struct lsh_object *) i->g);
  mark((struct lsh_object *) i->x);
}
struct lsh_class command_S_continuation_class =
{
  STATIC_HEADER,
  &(command_frame_class),
  "command_S_continuation",
  sizeof(struct command_S_continuation),
  do_command_S_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_C_continuation:command_frame
*/
#ifndef GABA_DEFINE
struct command_C_continuation
{
  struct command_frame super;
  struct lsh_object *y;
};
extern struct lsh_class command_C_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_command_C_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct command_C_continuation *i = (struct command_C_continuation *) o;
  mark((struct lsh_object *) i->y);
}
struct lsh_class command_C_continuation_class =
{
  STATIC_HEADER,
  &(command_frame_class),
  "command_C_continuation",
  sizeof(struct command_C_continuation),
  do_command_C_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

