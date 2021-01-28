/*
CLASS:connection_command:command
*/
#ifndef GABA_DEFINE
struct connection_command
{
  struct command super;
  struct ssh_connection *connection;
};
extern struct lsh_class connection_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_connection_command_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct connection_command *i = (struct connection_command *) o;
  mark((struct lsh_object *) i->connection);
}
struct lsh_class connection_command_class =
{
  STATIC_HEADER,
  &(command_class),
  "connection_command",
  sizeof(struct connection_command),
  do_connection_command_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

