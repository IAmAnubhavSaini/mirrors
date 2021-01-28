/*
CLASS:exit_callback:
*/
#ifndef GABA_DEFINE
struct exit_callback
{
  struct lsh_object super;
  void (*(exit))(struct exit_callback *self,int signaled,int core,int value);
};
extern struct lsh_class exit_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class exit_callback_class =
{
  STATIC_HEADER,
  NULL,
  "exit_callback",
  sizeof(struct exit_callback),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:reaper:
*/
#ifndef GABA_DEFINE
struct reaper
{
  struct lsh_object super;
  void (*(reap))(struct reaper *self,pid_t pid,struct exit_callback *callback);
  struct alist *children;
};
extern struct lsh_class reaper_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_reaper_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct reaper *i = (struct reaper *) o;
  mark((struct lsh_object *) i->children);
}
struct lsh_class reaper_class =
{
  STATIC_HEADER,
  NULL,
  "reaper",
  sizeof(struct reaper),
  do_reaper_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

