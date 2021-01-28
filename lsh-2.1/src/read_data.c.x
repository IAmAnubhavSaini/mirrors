/*
CLASS:read_data:io_consuming_read
*/
#ifndef GABA_DEFINE
struct read_data
{
  struct io_consuming_read super;
  struct ssh_channel *channel;
};
extern struct lsh_class read_data_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_read_data_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct read_data *i = (struct read_data *) o;
  mark((struct lsh_object *) i->channel);
}
struct lsh_class read_data_class =
{
  STATIC_HEADER,
  &(io_consuming_read_class),
  "read_data",
  sizeof(struct read_data),
  do_read_data_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

