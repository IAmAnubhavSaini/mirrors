/*
CLASS:spki_context:
*/
#ifndef GABA_DEFINE
struct spki_context
{
  struct lsh_object super;
  struct alist *algorithms;
  struct spki_acl_db db;
};
extern struct lsh_class spki_context_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_spki_context_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct spki_context *i = (struct spki_context *) o;
  mark((struct lsh_object *) i->algorithms);
  do_spki_acl_db_mark(&(i->db),
    mark);
}
static void
do_spki_context_free(struct lsh_object *o)
{
  struct spki_context *i = (struct spki_context *) o;
  do_spki_acl_db_free(&(i->db));
}
struct lsh_class spki_context_class =
{
  STATIC_HEADER,
  NULL,
  "spki_context",
  sizeof(struct spki_context),
  do_spki_context_mark,
  do_spki_context_free,
};
#endif /* !GABA_DECLARE */

