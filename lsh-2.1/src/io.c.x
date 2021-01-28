/*
CLASS:lsh_signal_handler:resource
*/
#ifndef GABA_DEFINE
struct lsh_signal_handler
{
  struct resource super;
  int signum;
  struct lsh_callback *action;
};
extern struct lsh_class lsh_signal_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_signal_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct lsh_signal_handler *i = (struct lsh_signal_handler *) o;
  mark((struct lsh_object *) i->action);
}
struct lsh_class lsh_signal_handler_class =
{
  STATIC_HEADER,
  &(resource_class),
  "lsh_signal_handler",
  sizeof(struct lsh_signal_handler),
  do_lsh_signal_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:lsh_callout:resource
*/
#ifndef GABA_DEFINE
struct lsh_callout
{
  struct resource super;
  struct timeval when;
  struct lsh_callback *action;
};
extern struct lsh_class lsh_callout_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_callout_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct lsh_callout *i = (struct lsh_callout *) o;
  mark((struct lsh_object *) i->action);
}
struct lsh_class lsh_callout_class =
{
  STATIC_HEADER,
  &(resource_class),
  "lsh_callout",
  sizeof(struct lsh_callout),
  do_lsh_callout_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:io_listen_callback:io_callback
*/
#ifndef GABA_DEFINE
struct io_listen_callback
{
  struct io_callback super;
  struct command *c;
  struct exception_handler *e;
};
extern struct lsh_class io_listen_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_io_listen_callback_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct io_listen_callback *i = (struct io_listen_callback *) o;
  mark((struct lsh_object *) i->c);
  mark((struct lsh_object *) i->e);
}
struct lsh_class io_listen_callback_class =
{
  STATIC_HEADER,
  &(io_callback_class),
  "io_listen_callback",
  sizeof(struct io_listen_callback),
  do_io_listen_callback_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:io_connect_callback:io_callback
*/
#ifndef GABA_DEFINE
struct io_connect_callback
{
  struct io_callback super;
  struct command_continuation *c;
};
extern struct lsh_class io_connect_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_io_connect_callback_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct io_connect_callback *i = (struct io_connect_callback *) o;
  mark((struct lsh_object *) i->c);
}
struct lsh_class io_connect_callback_class =
{
  STATIC_HEADER,
  &(io_callback_class),
  "io_connect_callback",
  sizeof(struct io_connect_callback),
  do_io_connect_callback_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:connect_list_callback:io_callback
*/
#ifndef GABA_DEFINE
struct connect_list_callback
{
  struct io_callback super;
  struct connect_list_state *state;
  unsigned index;
  struct command_continuation *c;
  struct exception_handler *e;
};
extern struct lsh_class connect_list_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_connect_list_callback_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct connect_list_callback *i = (struct connect_list_callback *) o;
  mark((struct lsh_object *) i->state);
  mark((struct lsh_object *) i->c);
  mark((struct lsh_object *) i->e);
}
struct lsh_class connect_list_callback_class =
{
  STATIC_HEADER,
  &(io_callback_class),
  "connect_list_callback",
  sizeof(struct connect_list_callback),
  do_connect_list_callback_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:write_only_file:abstract_write
*/
#ifndef GABA_DEFINE
struct write_only_file
{
  struct abstract_write super;
  int fd;
  struct exception_handler *e;
};
extern struct lsh_class write_only_file_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_write_only_file_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct write_only_file *i = (struct write_only_file *) o;
  mark((struct lsh_object *) i->e);
}
struct lsh_class write_only_file_class =
{
  STATIC_HEADER,
  &(abstract_write_class),
  "write_only_file",
  sizeof(struct write_only_file),
  do_write_only_file_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:exc_finish_read_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct exc_finish_read_handler
{
  struct exception_handler super;
  struct lsh_fd *fd;
};
extern struct lsh_class exc_finish_read_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_exc_finish_read_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct exc_finish_read_handler *i = (struct exc_finish_read_handler *) o;
  mark((struct lsh_object *) i->fd);
}
struct lsh_class exc_finish_read_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "exc_finish_read_handler",
  sizeof(struct exc_finish_read_handler),
  do_exc_finish_read_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

