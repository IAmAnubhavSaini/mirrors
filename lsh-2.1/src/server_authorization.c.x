/*
CLASS:authorization_db:lookup_verifier
*/
#ifndef GABA_DEFINE
struct authorization_db
{
  struct lookup_verifier super;
  struct lsh_string *index_name;
  const struct hash_algorithm *hashalgo;
};
extern struct lsh_class authorization_db_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_authorization_db_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct authorization_db *i = (struct authorization_db *) o;
  mark((struct lsh_object *) i->hashalgo);
}
static void
do_authorization_db_free(struct lsh_object *o)
{
  struct authorization_db *i = (struct authorization_db *) o;
  lsh_string_free(i->index_name);
}
struct lsh_class authorization_db_class =
{
  STATIC_HEADER,
  &(lookup_verifier_class),
  "authorization_db",
  sizeof(struct authorization_db),
  do_authorization_db_mark,
  do_authorization_db_free,
};
#endif /* !GABA_DECLARE */

