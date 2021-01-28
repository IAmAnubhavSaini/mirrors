/*
CLASS:forwarded_port:
*/
#ifndef GABA_DEFINE
struct forwarded_port
{
  struct lsh_object super;
  struct address_info *listen;
};
extern struct lsh_class forwarded_port_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_forwarded_port_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct forwarded_port *i = (struct forwarded_port *) o;
  mark((struct lsh_object *) i->listen);
}
struct lsh_class forwarded_port_class =
{
  STATIC_HEADER,
  NULL,
  "forwarded_port",
  sizeof(struct forwarded_port),
  do_forwarded_port_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:local_port:forwarded_port
*/
#ifndef GABA_DEFINE
struct local_port
{
  struct forwarded_port super;
  struct lsh_fd *socket;
};
extern struct lsh_class local_port_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_local_port_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct local_port *i = (struct local_port *) o;
  mark((struct lsh_object *) i->socket);
}
struct lsh_class local_port_class =
{
  STATIC_HEADER,
  &(forwarded_port_class),
  "local_port",
  sizeof(struct local_port),
  do_local_port_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:remote_port:forwarded_port
*/
#ifndef GABA_DEFINE
struct remote_port
{
  struct forwarded_port super;
  struct command *callback;
};
extern struct lsh_class remote_port_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_remote_port_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct remote_port *i = (struct remote_port *) o;
  mark((struct lsh_object *) i->callback);
}
struct lsh_class remote_port_class =
{
  STATIC_HEADER,
  &(forwarded_port_class),
  "remote_port",
  sizeof(struct remote_port),
  do_remote_port_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

