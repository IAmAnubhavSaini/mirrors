/*
CLASS:read_line:read_handler
*/
#ifndef GABA_DEFINE
struct read_line
{
  struct read_handler super;
  struct line_handler *handler;
  struct exception_handler *e;
  uint32_t pos;
  struct lsh_string *buffer;
};
extern struct lsh_class read_line_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_read_line_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct read_line *i = (struct read_line *) o;
  mark((struct lsh_object *) i->handler);
  mark((struct lsh_object *) i->e);
}
static void
do_read_line_free(struct lsh_object *o)
{
  struct read_line *i = (struct read_line *) o;
  lsh_string_free(i->buffer);
}
struct lsh_class read_line_class =
{
  STATIC_HEADER,
  &(read_handler_class),
  "read_line",
  sizeof(struct read_line),
  do_read_line_mark,
  do_read_line_free,
};
#endif /* !GABA_DECLARE */

