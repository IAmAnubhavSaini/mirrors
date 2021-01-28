/*
CLASS:lsh_callback:
*/
#ifndef GABA_DEFINE
struct lsh_callback
{
  struct lsh_object super;
  void (*(f))(struct lsh_callback *self);
};
extern struct lsh_class lsh_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class lsh_callback_class =
{
  STATIC_HEADER,
  NULL,
  "lsh_callback",
  sizeof(struct lsh_callback),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:io_callback:
*/
#ifndef GABA_DEFINE
struct io_callback
{
  struct lsh_object super;
  void (*(f))(struct io_callback *self,struct lsh_fd *fd);
};
extern struct lsh_class io_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class io_callback_class =
{
  STATIC_HEADER,
  NULL,
  "io_callback",
  sizeof(struct io_callback),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:lsh_fd:resource
*/
#ifndef GABA_DEFINE
struct lsh_fd
{
  struct resource super;
  int fd;
  enum io_type type;
  const char * label;
  struct exception_handler *e;
  struct lsh_callback *close_callback;
  int want_read;
  struct io_callback *read;
  int want_write;
  struct io_callback *write;
  struct write_buffer *write_buffer;
};
extern struct lsh_class lsh_fd_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_fd_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct lsh_fd *i = (struct lsh_fd *) o;
  mark((struct lsh_object *) i->e);
  mark((struct lsh_object *) i->close_callback);
  mark((struct lsh_object *) i->read);
  mark((struct lsh_object *) i->write);
  mark((struct lsh_object *) i->write_buffer);
}
struct lsh_class lsh_fd_class =
{
  STATIC_HEADER,
  &(resource_class),
  "lsh_fd",
  sizeof(struct lsh_fd),
  do_lsh_fd_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:io_buffered_read:io_callback
*/
#ifndef GABA_DEFINE
struct io_buffered_read
{
  struct io_callback super;
  uint32_t buffer_size;
  struct read_handler *handler;
};
extern struct lsh_class io_buffered_read_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_io_buffered_read_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct io_buffered_read *i = (struct io_buffered_read *) o;
  mark((struct lsh_object *) i->handler);
}
struct lsh_class io_buffered_read_class =
{
  STATIC_HEADER,
  &(io_callback_class),
  "io_buffered_read",
  sizeof(struct io_buffered_read),
  do_io_buffered_read_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:io_consuming_read:io_callback
*/
#ifndef GABA_DEFINE
struct io_consuming_read
{
  struct io_callback super;
  uint32_t (*(query))(struct io_consuming_read *self);
  struct abstract_write *consumer;
};
extern struct lsh_class io_consuming_read_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_io_consuming_read_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct io_consuming_read *i = (struct io_consuming_read *) o;
  mark((struct lsh_object *) i->consumer);
}
struct lsh_class io_consuming_read_class =
{
  STATIC_HEADER,
  &(io_callback_class),
  "io_consuming_read",
  sizeof(struct io_consuming_read),
  do_io_consuming_read_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:address_info:
*/
#ifndef GABA_DEFINE
struct address_info
{
  struct lsh_object super;
  struct lsh_string *ip;
  uint32_t port;
};
extern struct lsh_class address_info_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_address_info_free(struct lsh_object *o)
{
  struct address_info *i = (struct address_info *) o;
  lsh_string_free(i->ip);
}
struct lsh_class address_info_class =
{
  STATIC_HEADER,
  NULL,
  "address_info",
  sizeof(struct address_info),
  NULL,
  do_address_info_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:local_info:
*/
#ifndef GABA_DEFINE
struct local_info
{
  struct lsh_object super;
  struct lsh_string *directory;
  struct lsh_string *name;
};
extern struct lsh_class local_info_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_local_info_free(struct lsh_object *o)
{
  struct local_info *i = (struct local_info *) o;
  lsh_string_free(i->directory);
  lsh_string_free(i->name);
}
struct lsh_class local_info_class =
{
  STATIC_HEADER,
  NULL,
  "local_info",
  sizeof(struct local_info),
  NULL,
  do_local_info_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:listen_value:
*/
#ifndef GABA_DEFINE
struct listen_value
{
  struct lsh_object super;
  struct lsh_fd *fd;
  struct address_info *peer;
  struct address_info *local;
};
extern struct lsh_class listen_value_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_listen_value_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct listen_value *i = (struct listen_value *) o;
  mark((struct lsh_object *) i->fd);
  mark((struct lsh_object *) i->peer);
  mark((struct lsh_object *) i->local);
}
struct lsh_class listen_value_class =
{
  STATIC_HEADER,
  NULL,
  "listen_value",
  sizeof(struct listen_value),
  do_listen_value_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:io_exception:exception
*/
#ifndef GABA_DEFINE
struct io_exception
{
  struct exception super;
  struct lsh_fd *fd;
  int error;
};
extern struct lsh_class io_exception_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_io_exception_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct io_exception *i = (struct io_exception *) o;
  mark((struct lsh_object *) i->fd);
}
struct lsh_class io_exception_class =
{
  STATIC_HEADER,
  &(exception_class),
  "io_exception",
  sizeof(struct io_exception),
  do_io_exception_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:connect_list_state:resource
*/
#ifndef GABA_DEFINE
struct connect_list_state
{
  struct resource super;
  struct addr_queue q;
  unsigned nfds;
  struct lsh_fd *((fds)[CONNECT_ATTEMPTS_LIMIT]);
};
extern struct lsh_class connect_list_state_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_connect_list_state_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct connect_list_state *i = (struct connect_list_state *) o;
  addr_queue_mark(&(i->q),
    mark);
  {
    unsigned k1;
    for(k1=0; k1<CONNECT_ATTEMPTS_LIMIT; k1++)
      mark((struct lsh_object *) (i->fds)[k1]);
  };
}
static void
do_connect_list_state_free(struct lsh_object *o)
{
  struct connect_list_state *i = (struct connect_list_state *) o;
  addr_queue_free(&(i->q));
}
struct lsh_class connect_list_state_class =
{
  STATIC_HEADER,
  &(resource_class),
  "connect_list_state",
  sizeof(struct connect_list_state),
  do_connect_list_state_mark,
  do_connect_list_state_free,
};
#endif /* !GABA_DECLARE */

