/*
CLASS:unix_process:lsh_process
*/
#ifndef GABA_DEFINE
struct unix_process
{
  struct lsh_process super;
  pid_t pid;
  int signal;
};
extern struct lsh_class unix_process_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class unix_process_class =
{
  STATIC_HEADER,
  &(lsh_process_class),
  "unix_process",
  sizeof(struct unix_process),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:logout_notice:exit_callback
*/
#ifndef GABA_DEFINE
struct logout_notice
{
  struct exit_callback super;
  struct resource *process;
  struct exit_callback *c;
};
extern struct lsh_class logout_notice_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_logout_notice_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct logout_notice *i = (struct logout_notice *) o;
  mark((struct lsh_object *) i->process);
  mark((struct lsh_object *) i->c);
}
struct lsh_class logout_notice_class =
{
  STATIC_HEADER,
  &(exit_callback_class),
  "logout_notice",
  sizeof(struct logout_notice),
  do_logout_notice_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:utmp_cleanup:exit_callback
*/
#ifndef GABA_DEFINE
struct utmp_cleanup
{
  struct exit_callback super;
  struct lsh_string *id;
  struct lsh_string *line;
  struct exit_callback *c;
};
extern struct lsh_class utmp_cleanup_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_utmp_cleanup_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct utmp_cleanup *i = (struct utmp_cleanup *) o;
  mark((struct lsh_object *) i->c);
}
static void
do_utmp_cleanup_free(struct lsh_object *o)
{
  struct utmp_cleanup *i = (struct utmp_cleanup *) o;
  lsh_string_free(i->id);
  lsh_string_free(i->line);
}
struct lsh_class utmp_cleanup_class =
{
  STATIC_HEADER,
  &(exit_callback_class),
  "utmp_cleanup",
  sizeof(struct utmp_cleanup),
  do_utmp_cleanup_mark,
  do_utmp_cleanup_free,
};
#endif /* !GABA_DECLARE */

