/*
CLASS:user_db:
*/
#ifndef GABA_DEFINE
struct user_db
{
  struct lsh_object super;
  struct lsh_user * (*(lookup))(struct user_db *self,struct lsh_string *name,int free);
};
extern struct lsh_class user_db_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class user_db_class =
{
  STATIC_HEADER,
  NULL,
  "user_db",
  sizeof(struct user_db),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:userauth:
*/
#ifndef GABA_DEFINE
struct userauth
{
  struct lsh_object super;
  void (*(authenticate))(struct userauth *self,struct ssh_connection *connection,struct lsh_string *username,uint32_t service,struct simple_buffer *args,struct command_continuation *c,struct exception_handler *e);
};
extern struct lsh_class userauth_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class userauth_class =
{
  STATIC_HEADER,
  NULL,
  "userauth",
  sizeof(struct userauth),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:userauth_service:command
*/
#ifndef GABA_DEFINE
struct userauth_service
{
  struct command super;
  struct int_list *advertised_methods;
  struct alist *methods;
  struct alist *services;
};
extern struct lsh_class userauth_service_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_userauth_service_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct userauth_service *i = (struct userauth_service *) o;
  mark((struct lsh_object *) i->advertised_methods);
  mark((struct lsh_object *) i->methods);
  mark((struct lsh_object *) i->services);
}
struct lsh_class userauth_service_class =
{
  STATIC_HEADER,
  &(command_class),
  "userauth_service",
  sizeof(struct userauth_service),
  do_userauth_service_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

