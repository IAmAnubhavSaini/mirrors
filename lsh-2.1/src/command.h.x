/*
CLASS:command_continuation:
*/
#ifndef GABA_DEFINE
struct command_continuation
{
  struct lsh_object super;
  void (*(c))(struct command_continuation *self,struct lsh_object *result);
};
extern struct lsh_class command_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class command_continuation_class =
{
  STATIC_HEADER,
  NULL,
  "command_continuation",
  sizeof(struct command_continuation),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command:
*/
#ifndef GABA_DEFINE
struct command
{
  struct lsh_object super;
  void (*(call))(struct command *self,struct lsh_object *arg,struct command_continuation *c,struct exception_handler *e);
};
extern struct lsh_class command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class command_class =
{
  STATIC_HEADER,
  NULL,
  "command",
  sizeof(struct command),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_2:command
*/
#ifndef GABA_DEFINE
struct command_2
{
  struct command super;
  void (*(invoke))(struct command_2 *self,struct lsh_object *a1,struct lsh_object *a2,struct command_continuation *c,struct exception_handler *e);
};
extern struct lsh_class command_2_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class command_2_class =
{
  STATIC_HEADER,
  &(command_class),
  "command_2",
  sizeof(struct command_2),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_3:command
*/
#ifndef GABA_DEFINE
struct command_3
{
  struct command super;
  void (*(invoke))(struct lsh_object *a1,struct lsh_object *a2,struct lsh_object *a3,struct command_continuation *c,struct exception_handler *e);
};
extern struct lsh_class command_3_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class command_3_class =
{
  STATIC_HEADER,
  &(command_class),
  "command_3",
  sizeof(struct command_3),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_4:command
*/
#ifndef GABA_DEFINE
struct command_4
{
  struct command super;
  void (*(invoke))(struct lsh_object *a1,struct lsh_object *a2,struct lsh_object *a3,struct lsh_object *a4,struct command_continuation *c,struct exception_handler *e);
};
extern struct lsh_class command_4_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class command_4_class =
{
  STATIC_HEADER,
  &(command_class),
  "command_4",
  sizeof(struct command_4),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_frame:command_continuation
*/
#ifndef GABA_DEFINE
struct command_frame
{
  struct command_continuation super;
  struct command_continuation *up;
  struct exception_handler *e;
};
extern struct lsh_class command_frame_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_command_frame_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct command_frame *i = (struct command_frame *) o;
  mark((struct lsh_object *) i->up);
  mark((struct lsh_object *) i->e);
}
struct lsh_class command_frame_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "command_frame",
  sizeof(struct command_frame),
  do_command_frame_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_context:
*/
#ifndef GABA_DEFINE
struct command_context
{
  struct lsh_object super;
  struct command_continuation *c;
  struct exception_handler *e;
};
extern struct lsh_class command_context_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_command_context_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct command_context *i = (struct command_context *) o;
  mark((struct lsh_object *) i->c);
  mark((struct lsh_object *) i->e);
}
struct lsh_class command_context_class =
{
  STATIC_HEADER,
  NULL,
  "command_context",
  sizeof(struct command_context),
  do_command_context_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:catch_report_collect:command
*/
#ifndef GABA_DEFINE
struct catch_report_collect
{
  struct command super;
  const struct report_exception_info *info;
};
extern struct lsh_class catch_report_collect_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_catch_report_collect_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct catch_report_collect *i = (struct catch_report_collect *) o;
  mark((struct lsh_object *) i->info);
}
struct lsh_class catch_report_collect_class =
{
  STATIC_HEADER,
  &(command_class),
  "catch_report_collect",
  sizeof(struct catch_report_collect),
  do_catch_report_collect_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

