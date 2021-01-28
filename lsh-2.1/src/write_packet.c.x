/*
CLASS:write_packet:abstract_write_pipe
*/
#ifndef GABA_DEFINE
struct write_packet
{
  struct abstract_write_pipe super;
  struct ssh_connection *connection;
  struct randomness *random;
  uint32_t sequence_number;
};
extern struct lsh_class write_packet_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_write_packet_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct write_packet *i = (struct write_packet *) o;
  mark((struct lsh_object *) i->connection);
  mark((struct lsh_object *) i->random);
}
struct lsh_class write_packet_class =
{
  STATIC_HEADER,
  &(abstract_write_pipe_class),
  "write_packet",
  sizeof(struct write_packet),
  do_write_packet_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

