/*
CLASS:userauth_publickey:userauth
*/
#ifndef GABA_DEFINE
struct userauth_publickey
{
  struct userauth super;
  struct user_db *db;
  struct alist *verifiers;
};
extern struct lsh_class userauth_publickey_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_userauth_publickey_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct userauth_publickey *i = (struct userauth_publickey *) o;
  mark((struct lsh_object *) i->db);
  mark((struct lsh_object *) i->verifiers);
}
struct lsh_class userauth_publickey_class =
{
  STATIC_HEADER,
  &(userauth_class),
  "userauth_publickey",
  sizeof(struct userauth_publickey),
  do_userauth_publickey_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

