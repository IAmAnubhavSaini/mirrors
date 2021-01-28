/*
CLASS:connection_line_handler:line_handler
*/
#ifndef GABA_DEFINE
struct connection_line_handler
{
  struct line_handler super;
  struct ssh_connection *connection;
};
extern struct lsh_class connection_line_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_connection_line_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct connection_line_handler *i = (struct connection_line_handler *) o;
  mark((struct lsh_object *) i->connection);
}
struct lsh_class connection_line_handler_class =
{
  STATIC_HEADER,
  &(line_handler_class),
  "connection_line_handler",
  sizeof(struct connection_line_handler),
  do_connection_line_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

