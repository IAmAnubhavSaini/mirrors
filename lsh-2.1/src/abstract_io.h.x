/*
CLASS:abstract_write:
*/
#ifndef GABA_DEFINE
struct abstract_write
{
  struct lsh_object super;
  void (*(write))(struct abstract_write *self,struct lsh_string *packet);
};
extern struct lsh_class abstract_write_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class abstract_write_class =
{
  STATIC_HEADER,
  NULL,
  "abstract_write",
  sizeof(struct abstract_write),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:abstract_write_pipe:abstract_write
*/
#ifndef GABA_DEFINE
struct abstract_write_pipe
{
  struct abstract_write super;
  struct abstract_write *next;
};
extern struct lsh_class abstract_write_pipe_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_abstract_write_pipe_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct abstract_write_pipe *i = (struct abstract_write_pipe *) o;
  mark((struct lsh_object *) i->next);
}
struct lsh_class abstract_write_pipe_class =
{
  STATIC_HEADER,
  &(abstract_write_class),
  "abstract_write_pipe",
  sizeof(struct abstract_write_pipe),
  do_abstract_write_pipe_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:read_handler:
*/
#ifndef GABA_DEFINE
struct read_handler
{
  struct lsh_object super;
  uint32_t (*(handler))(struct read_handler **self,uint32_t available,const uint8_t *data);
};
extern struct lsh_class read_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class read_handler_class =
{
  STATIC_HEADER,
  NULL,
  "read_handler",
  sizeof(struct read_handler),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

