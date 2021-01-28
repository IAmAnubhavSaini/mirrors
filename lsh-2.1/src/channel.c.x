/*
CLASS:exc_finish_channel_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct exc_finish_channel_handler
{
  struct exception_handler super;
  struct ssh_connection *connection;
  int dead;
  uint32_t channel_number;
};
extern struct lsh_class exc_finish_channel_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_exc_finish_channel_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct exc_finish_channel_handler *i = (struct exc_finish_channel_handler *) o;
  mark((struct lsh_object *) i->connection);
}
struct lsh_class exc_finish_channel_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "exc_finish_channel_handler",
  sizeof(struct exc_finish_channel_handler),
  do_exc_finish_channel_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:request_status:
*/
#ifndef GABA_DEFINE
struct request_status
{
  struct lsh_object super;
  int status;
};
extern struct lsh_class request_status_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class request_status_class =
{
  STATIC_HEADER,
  NULL,
  "request_status",
  sizeof(struct request_status),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:global_request_continuation:command_continuation
*/
#ifndef GABA_DEFINE
struct global_request_continuation
{
  struct command_continuation super;
  struct ssh_connection *connection;
  struct request_status *active;
};
extern struct lsh_class global_request_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_global_request_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct global_request_continuation *i = (struct global_request_continuation *) o;
  mark((struct lsh_object *) i->connection);
  mark((struct lsh_object *) i->active);
}
struct lsh_class global_request_continuation_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "global_request_continuation",
  sizeof(struct global_request_continuation),
  do_global_request_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:global_request_exception_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct global_request_exception_handler
{
  struct exception_handler super;
  struct ssh_connection *connection;
  struct request_status *active;
};
extern struct lsh_class global_request_exception_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_global_request_exception_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct global_request_exception_handler *i = (struct global_request_exception_handler *) o;
  mark((struct lsh_object *) i->connection);
  mark((struct lsh_object *) i->active);
}
struct lsh_class global_request_exception_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "global_request_exception_handler",
  sizeof(struct global_request_exception_handler),
  do_global_request_exception_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:channel_request_continuation:command_continuation
*/
#ifndef GABA_DEFINE
struct channel_request_continuation
{
  struct command_continuation super;
  struct ssh_channel *channel;
  struct request_status *active;
};
extern struct lsh_class channel_request_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_channel_request_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct channel_request_continuation *i = (struct channel_request_continuation *) o;
  mark((struct lsh_object *) i->channel);
  mark((struct lsh_object *) i->active);
}
struct lsh_class channel_request_continuation_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "channel_request_continuation",
  sizeof(struct channel_request_continuation),
  do_channel_request_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:channel_request_exception_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct channel_request_exception_handler
{
  struct exception_handler super;
  struct ssh_channel *channel;
  struct request_status *active;
};
extern struct lsh_class channel_request_exception_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_channel_request_exception_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct channel_request_exception_handler *i = (struct channel_request_exception_handler *) o;
  mark((struct lsh_object *) i->channel);
  mark((struct lsh_object *) i->active);
}
struct lsh_class channel_request_exception_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "channel_request_exception_handler",
  sizeof(struct channel_request_exception_handler),
  do_channel_request_exception_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:channel_open_continuation:command_continuation
*/
#ifndef GABA_DEFINE
struct channel_open_continuation
{
  struct command_continuation super;
  struct ssh_connection *connection;
  uint32_t local_channel_number;
  uint32_t remote_channel_number;
  uint32_t send_window_size;
  uint32_t send_max_packet;
};
extern struct lsh_class channel_open_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_channel_open_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct channel_open_continuation *i = (struct channel_open_continuation *) o;
  mark((struct lsh_object *) i->connection);
}
struct lsh_class channel_open_continuation_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "channel_open_continuation",
  sizeof(struct channel_open_continuation),
  do_channel_open_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:exc_channel_open_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct exc_channel_open_handler
{
  struct exception_handler super;
  struct ssh_connection *connection;
  uint32_t local_channel_number;
  uint32_t remote_channel_number;
};
extern struct lsh_class exc_channel_open_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_exc_channel_open_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct exc_channel_open_handler *i = (struct exc_channel_open_handler *) o;
  mark((struct lsh_object *) i->connection);
}
struct lsh_class exc_channel_open_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "exc_channel_open_handler",
  sizeof(struct exc_channel_open_handler),
  do_exc_channel_open_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:channel_write:abstract_write
*/
#ifndef GABA_DEFINE
struct channel_write
{
  struct abstract_write super;
  struct ssh_channel *channel;
};
extern struct lsh_class channel_write_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_channel_write_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct channel_write *i = (struct channel_write *) o;
  mark((struct lsh_object *) i->channel);
}
struct lsh_class channel_write_class =
{
  STATIC_HEADER,
  &(abstract_write_class),
  "channel_write",
  sizeof(struct channel_write),
  do_channel_write_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:channel_write_extended:channel_write
*/
#ifndef GABA_DEFINE
struct channel_write_extended
{
  struct channel_write super;
  uint32_t type;
};
extern struct lsh_class channel_write_extended_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class channel_write_extended_class =
{
  STATIC_HEADER,
  &(channel_write_class),
  "channel_write_extended",
  sizeof(struct channel_write_extended),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:channel_close_callback:lsh_callback
*/
#ifndef GABA_DEFINE
struct channel_close_callback
{
  struct lsh_callback super;
  struct ssh_channel *channel;
};
extern struct lsh_class channel_close_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_channel_close_callback_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct channel_close_callback *i = (struct channel_close_callback *) o;
  mark((struct lsh_object *) i->channel);
}
struct lsh_class channel_close_callback_class =
{
  STATIC_HEADER,
  &(lsh_callback_class),
  "channel_close_callback",
  sizeof(struct channel_close_callback),
  do_channel_close_callback_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:channel_io_exception_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct channel_io_exception_handler
{
  struct exception_handler super;
  struct ssh_channel *channel;
  const char * prefix;
  int silent;
};
extern struct lsh_class channel_io_exception_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_channel_io_exception_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct channel_io_exception_handler *i = (struct channel_io_exception_handler *) o;
  mark((struct lsh_object *) i->channel);
}
struct lsh_class channel_io_exception_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "channel_io_exception_handler",
  sizeof(struct channel_io_exception_handler),
  do_channel_io_exception_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

