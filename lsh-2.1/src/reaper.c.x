/*
CLASS:reaper_callback:lsh_callback
*/
#ifndef GABA_DEFINE
struct reaper_callback
{
  struct lsh_callback super;
  struct reaper *reaper;
};
extern struct lsh_class reaper_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_reaper_callback_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct reaper_callback *i = (struct reaper_callback *) o;
  mark((struct lsh_object *) i->reaper);
}
struct lsh_class reaper_callback_class =
{
  STATIC_HEADER,
  &(lsh_callback_class),
  "reaper_callback",
  sizeof(struct reaper_callback),
  do_reaper_callback_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

