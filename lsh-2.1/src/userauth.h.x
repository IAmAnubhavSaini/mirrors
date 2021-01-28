/*
CLASS:userauth_special_exception:exception
*/
#ifndef GABA_DEFINE
struct userauth_special_exception
{
  struct exception super;
  struct lsh_string *reply;
};
extern struct lsh_class userauth_special_exception_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_userauth_special_exception_free(struct lsh_object *o)
{
  struct userauth_special_exception *i = (struct userauth_special_exception *) o;
  lsh_string_free(i->reply);
}
struct lsh_class userauth_special_exception_class =
{
  STATIC_HEADER,
  &(exception_class),
  "userauth_special_exception",
  sizeof(struct userauth_special_exception),
  NULL,
  do_userauth_special_exception_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:lsh_process:resource
*/
#ifndef GABA_DEFINE
struct lsh_process
{
  struct resource super;
  int (*(signal))(struct lsh_process *self,int);
};
extern struct lsh_class lsh_process_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class lsh_process_class =
{
  STATIC_HEADER,
  &(resource_class),
  "lsh_process",
  sizeof(struct lsh_process),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:lsh_user:
*/
#ifndef GABA_DEFINE
struct lsh_user
{
  struct lsh_object super;
  struct lsh_string *name;
  uid_t uid;
  void (*(verify_password))(struct lsh_user *self,struct lsh_string *pw,struct command_continuation *c,struct exception_handler *e);
  int (*(file_exists))(struct lsh_user *self,struct lsh_string *name,int free);
  const struct exception * (*(read_file))(struct lsh_user *self,const char *name,int secret,uint32_t limit,struct abstract_write *c);
  struct lsh_process * (*(spawn))(struct lsh_user *self,struct spawn_info *info,struct exit_callback *c);
};
extern struct lsh_class lsh_user_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_user_free(struct lsh_object *o)
{
  struct lsh_user *i = (struct lsh_user *) o;
  lsh_string_free(i->name);
}
struct lsh_class lsh_user_class =
{
  STATIC_HEADER,
  NULL,
  "lsh_user",
  sizeof(struct lsh_user),
  NULL,
  do_lsh_user_free,
};
#endif /* !GABA_DECLARE */

