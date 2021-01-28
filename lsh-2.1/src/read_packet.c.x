/*
CLASS:read_packet:read_handler
*/
#ifndef GABA_DEFINE
struct read_packet
{
  struct read_handler super;
  int state;
  uint32_t sequence_number;
  uint32_t pos;
  struct lsh_string *block_buffer;
  struct lsh_string *mac_buffer;
  struct lsh_string *mac_computed;
  struct lsh_string *packet_buffer;
  uint32_t payload_length;
  uint32_t crypt_pos;
  struct abstract_write *handler;
  struct ssh_connection *connection;
};
extern struct lsh_class read_packet_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_read_packet_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct read_packet *i = (struct read_packet *) o;
  mark((struct lsh_object *) i->handler);
  mark((struct lsh_object *) i->connection);
}
static void
do_read_packet_free(struct lsh_object *o)
{
  struct read_packet *i = (struct read_packet *) o;
  lsh_string_free(i->block_buffer);
  lsh_string_free(i->mac_buffer);
  lsh_string_free(i->mac_computed);
  lsh_string_free(i->packet_buffer);
}
struct lsh_class read_packet_class =
{
  STATIC_HEADER,
  &(read_handler_class),
  "read_packet",
  sizeof(struct read_packet),
  do_read_packet_mark,
  do_read_packet_free,
};
#endif /* !GABA_DECLARE */

