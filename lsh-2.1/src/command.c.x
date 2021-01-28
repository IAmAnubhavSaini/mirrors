/*
CLASS:command_apply:command_frame
*/
#ifndef GABA_DEFINE
struct command_apply
{
  struct command_frame super;
  struct command *f;
};
extern struct lsh_class command_apply_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_command_apply_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct command_apply *i = (struct command_apply *) o;
  mark((struct lsh_object *) i->f);
}
struct lsh_class command_apply_class =
{
  STATIC_HEADER,
  &(command_frame_class),
  "command_apply",
  sizeof(struct command_apply),
  do_command_apply_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:gaba_continuation:command_continuation
*/
#ifndef GABA_DEFINE
struct gaba_continuation
{
  struct command_continuation super;
  struct lsh_object *value;
};
extern struct lsh_class gaba_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_gaba_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct gaba_continuation *i = (struct gaba_continuation *) o;
  mark((struct lsh_object *) i->value);
}
struct lsh_class gaba_continuation_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "gaba_continuation",
  sizeof(struct gaba_continuation),
  do_gaba_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_2_invoke:command
*/
#ifndef GABA_DEFINE
struct command_2_invoke
{
  struct command super;
  struct command_2 *f;
  struct lsh_object *a1;
};
extern struct lsh_class command_2_invoke_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_command_2_invoke_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct command_2_invoke *i = (struct command_2_invoke *) o;
  mark((struct lsh_object *) i->f);
  mark((struct lsh_object *) i->a1);
}
struct lsh_class command_2_invoke_class =
{
  STATIC_HEADER,
  &(command_class),
  "command_2_invoke",
  sizeof(struct command_2_invoke),
  do_command_2_invoke_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_3_invoke_2:command
*/
#ifndef GABA_DEFINE
struct command_3_invoke_2
{
  struct command super;
  struct command_3 *f;
  struct lsh_object *a1;
  struct lsh_object *a2;
};
extern struct lsh_class command_3_invoke_2_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_command_3_invoke_2_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct command_3_invoke_2 *i = (struct command_3_invoke_2 *) o;
  mark((struct lsh_object *) i->f);
  mark((struct lsh_object *) i->a1);
  mark((struct lsh_object *) i->a2);
}
struct lsh_class command_3_invoke_2_class =
{
  STATIC_HEADER,
  &(command_class),
  "command_3_invoke_2",
  sizeof(struct command_3_invoke_2),
  do_command_3_invoke_2_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_3_invoke:command
*/
#ifndef GABA_DEFINE
struct command_3_invoke
{
  struct command super;
  struct command_3 *f;
  struct lsh_object *a1;
};
extern struct lsh_class command_3_invoke_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_command_3_invoke_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct command_3_invoke *i = (struct command_3_invoke *) o;
  mark((struct lsh_object *) i->f);
  mark((struct lsh_object *) i->a1);
}
struct lsh_class command_3_invoke_class =
{
  STATIC_HEADER,
  &(command_class),
  "command_3_invoke",
  sizeof(struct command_3_invoke),
  do_command_3_invoke_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_4_invoke_3:command
*/
#ifndef GABA_DEFINE
struct command_4_invoke_3
{
  struct command super;
  struct command_4 *f;
  struct lsh_object *a1;
  struct lsh_object *a2;
  struct lsh_object *a3;
};
extern struct lsh_class command_4_invoke_3_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_command_4_invoke_3_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct command_4_invoke_3 *i = (struct command_4_invoke_3 *) o;
  mark((struct lsh_object *) i->f);
  mark((struct lsh_object *) i->a1);
  mark((struct lsh_object *) i->a2);
  mark((struct lsh_object *) i->a3);
}
struct lsh_class command_4_invoke_3_class =
{
  STATIC_HEADER,
  &(command_class),
  "command_4_invoke_3",
  sizeof(struct command_4_invoke_3),
  do_command_4_invoke_3_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_4_invoke_2:command
*/
#ifndef GABA_DEFINE
struct command_4_invoke_2
{
  struct command super;
  struct command_4 *f;
  struct lsh_object *a1;
  struct lsh_object *a2;
};
extern struct lsh_class command_4_invoke_2_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_command_4_invoke_2_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct command_4_invoke_2 *i = (struct command_4_invoke_2 *) o;
  mark((struct lsh_object *) i->f);
  mark((struct lsh_object *) i->a1);
  mark((struct lsh_object *) i->a2);
}
struct lsh_class command_4_invoke_2_class =
{
  STATIC_HEADER,
  &(command_class),
  "command_4_invoke_2",
  sizeof(struct command_4_invoke_2),
  do_command_4_invoke_2_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:command_4_invoke:command
*/
#ifndef GABA_DEFINE
struct command_4_invoke
{
  struct command super;
  struct command_4 *f;
  struct lsh_object *a1;
};
extern struct lsh_class command_4_invoke_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_command_4_invoke_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct command_4_invoke *i = (struct command_4_invoke *) o;
  mark((struct lsh_object *) i->f);
  mark((struct lsh_object *) i->a1);
}
struct lsh_class command_4_invoke_class =
{
  STATIC_HEADER,
  &(command_class),
  "command_4_invoke",
  sizeof(struct command_4_invoke),
  do_command_4_invoke_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:trace_continuation:command_continuation
*/
#ifndef GABA_DEFINE
struct trace_continuation
{
  struct command_continuation super;
  const char * name;
  struct command_continuation *real;
};
extern struct lsh_class trace_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_trace_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct trace_continuation *i = (struct trace_continuation *) o;
  mark((struct lsh_object *) i->real);
}
struct lsh_class trace_continuation_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "trace_continuation",
  sizeof(struct trace_continuation),
  do_trace_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:trace_command:command
*/
#ifndef GABA_DEFINE
struct trace_command
{
  struct command super;
  const char * name;
  struct command *real;
};
extern struct lsh_class trace_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_trace_command_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct trace_command *i = (struct trace_command *) o;
  mark((struct lsh_object *) i->real);
}
struct lsh_class trace_command_class =
{
  STATIC_HEADER,
  &(command_class),
  "trace_command",
  sizeof(struct trace_command),
  do_trace_command_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:parallell_progn:command
*/
#ifndef GABA_DEFINE
struct parallell_progn
{
  struct command super;
  struct object_list *body;
};
extern struct lsh_class parallell_progn_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_parallell_progn_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct parallell_progn *i = (struct parallell_progn *) o;
  mark((struct lsh_object *) i->body);
}
struct lsh_class parallell_progn_class =
{
  STATIC_HEADER,
  &(command_class),
  "parallell_progn",
  sizeof(struct parallell_progn),
  do_parallell_progn_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:catch_handler_info:
*/
#ifndef GABA_DEFINE
struct catch_handler_info
{
  struct lsh_object super;
  uint32_t mask;
  uint32_t value;
  int ignore_value;
  struct command *handler;
};
extern struct lsh_class catch_handler_info_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_catch_handler_info_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct catch_handler_info *i = (struct catch_handler_info *) o;
  mark((struct lsh_object *) i->handler);
}
struct lsh_class catch_handler_info_class =
{
  STATIC_HEADER,
  NULL,
  "catch_handler_info",
  sizeof(struct catch_handler_info),
  do_catch_handler_info_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:catch_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct catch_handler
{
  struct exception_handler super;
  struct command_continuation *c;
  struct catch_handler_info *info;
};
extern struct lsh_class catch_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_catch_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct catch_handler *i = (struct catch_handler *) o;
  mark((struct lsh_object *) i->c);
  mark((struct lsh_object *) i->info);
}
struct lsh_class catch_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "catch_handler",
  sizeof(struct catch_handler),
  do_catch_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:catch_apply:command
*/
#ifndef GABA_DEFINE
struct catch_apply
{
  struct command super;
  struct catch_handler_info *info;
  struct command *body;
};
extern struct lsh_class catch_apply_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_catch_apply_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct catch_apply *i = (struct catch_apply *) o;
  mark((struct lsh_object *) i->info);
  mark((struct lsh_object *) i->body);
}
struct lsh_class catch_apply_class =
{
  STATIC_HEADER,
  &(command_class),
  "catch_apply",
  sizeof(struct catch_apply),
  do_catch_apply_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:catch_report_apply:command
*/
#ifndef GABA_DEFINE
struct catch_report_apply
{
  struct command super;
  const struct report_exception_info *info;
  struct command *body;
};
extern struct lsh_class catch_report_apply_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_catch_report_apply_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct catch_report_apply *i = (struct catch_report_apply *) o;
  mark((struct lsh_object *) i->info);
  mark((struct lsh_object *) i->body);
}
struct lsh_class catch_report_apply_class =
{
  STATIC_HEADER,
  &(command_class),
  "catch_report_apply",
  sizeof(struct catch_report_apply),
  do_catch_report_apply_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:protect_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct protect_handler
{
  struct exception_handler super;
  struct resource *resource;
};
extern struct lsh_class protect_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_protect_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct protect_handler *i = (struct protect_handler *) o;
  mark((struct lsh_object *) i->resource);
}
struct lsh_class protect_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "protect_handler",
  sizeof(struct protect_handler),
  do_protect_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

