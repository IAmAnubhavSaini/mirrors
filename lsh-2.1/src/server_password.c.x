/*
CLASS:userauth_password:userauth
*/
#ifndef GABA_DEFINE
struct userauth_password
{
  struct userauth super;
  struct user_db *db;
};
extern struct lsh_class userauth_password_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_userauth_password_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct userauth_password *i = (struct userauth_password *) o;
  mark((struct lsh_object *) i->db);
}
struct lsh_class userauth_password_class =
{
  STATIC_HEADER,
  &(userauth_class),
  "userauth_password",
  sizeof(struct userauth_password),
  do_userauth_password_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

