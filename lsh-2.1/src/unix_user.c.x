/*
CLASS:unix_user:lsh_user
*/
#ifndef GABA_DEFINE
struct unix_user
{
  struct lsh_user super;
  gid_t gid;
  struct unix_user_db *ctx;
  struct lsh_string *passwd;
  struct lsh_string *home;
  struct lsh_string *shell;
};
extern struct lsh_class unix_user_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_unix_user_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct unix_user *i = (struct unix_user *) o;
  mark((struct lsh_object *) i->ctx);
}
static void
do_unix_user_free(struct lsh_object *o)
{
  struct unix_user *i = (struct unix_user *) o;
  lsh_string_free(i->passwd);
  lsh_string_free(i->home);
  lsh_string_free(i->shell);
}
struct lsh_class unix_user_class =
{
  STATIC_HEADER,
  &(lsh_user_class),
  "unix_user",
  sizeof(struct unix_user),
  do_unix_user_mark,
  do_unix_user_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:pwhelper_callback:exit_callback
*/
#ifndef GABA_DEFINE
struct pwhelper_callback
{
  struct exit_callback super;
  struct unix_user *user;
  struct command_continuation *c;
  struct exception_handler *e;
};
extern struct lsh_class pwhelper_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_pwhelper_callback_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct pwhelper_callback *i = (struct pwhelper_callback *) o;
  mark((struct lsh_object *) i->user);
  mark((struct lsh_object *) i->c);
  mark((struct lsh_object *) i->e);
}
struct lsh_class pwhelper_callback_class =
{
  STATIC_HEADER,
  &(exit_callback_class),
  "pwhelper_callback",
  sizeof(struct pwhelper_callback),
  do_pwhelper_callback_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:exc_read_user_file_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct exc_read_user_file_handler
{
  struct exception_handler super;
  struct abstract_write *c;
};
extern struct lsh_class exc_read_user_file_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_exc_read_user_file_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct exc_read_user_file_handler *i = (struct exc_read_user_file_handler *) o;
  mark((struct lsh_object *) i->c);
}
struct lsh_class exc_read_user_file_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "exc_read_user_file_handler",
  sizeof(struct exc_read_user_file_handler),
  do_exc_read_user_file_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:unix_user_db:user_db
*/
#ifndef GABA_DEFINE
struct unix_user_db
{
  struct user_db super;
  struct reaper *reaper;
  const char * pw_helper;
  const char * login_shell;
  int allow_root;
};
extern struct lsh_class unix_user_db_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_unix_user_db_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct unix_user_db *i = (struct unix_user_db *) o;
  mark((struct lsh_object *) i->reaper);
}
struct lsh_class unix_user_db_class =
{
  STATIC_HEADER,
  &(user_db_class),
  "unix_user_db",
  sizeof(struct unix_user_db),
  do_unix_user_db_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:unix_user_self:lsh_user
*/
#ifndef GABA_DEFINE
struct unix_user_self
{
  struct lsh_user super;
  struct reaper *reaper;
  const char * shell;
  const char * home;
};
extern struct lsh_class unix_user_self_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_unix_user_self_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct unix_user_self *i = (struct unix_user_self *) o;
  mark((struct lsh_object *) i->reaper);
}
struct lsh_class unix_user_self_class =
{
  STATIC_HEADER,
  &(lsh_user_class),
  "unix_user_self",
  sizeof(struct unix_user_self),
  do_unix_user_self_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

