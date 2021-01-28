/*
CLASS:read_file:read_handler
*/
#ifndef GABA_DEFINE
struct read_file
{
  struct read_handler super;
  struct abstract_write *c;
  struct lsh_string *buffer;
  uint32_t pos;
};
extern struct lsh_class read_file_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_read_file_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct read_file *i = (struct read_file *) o;
  mark((struct lsh_object *) i->c);
}
static void
do_read_file_free(struct lsh_object *o)
{
  struct read_file *i = (struct read_file *) o;
  lsh_string_free(i->buffer);
}
struct lsh_class read_file_class =
{
  STATIC_HEADER,
  &(read_handler_class),
  "read_file",
  sizeof(struct read_file),
  do_read_file_mark,
  do_read_file_free,
};
#endif /* !GABA_DECLARE */

