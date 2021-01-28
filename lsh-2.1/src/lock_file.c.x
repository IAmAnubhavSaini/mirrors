/*
CLASS:lsh_file_lock:resource
*/
#ifndef GABA_DEFINE
struct lsh_file_lock
{
  struct resource super;
  struct lsh_file_lock_info *info;
};
extern struct lsh_class lsh_file_lock_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_file_lock_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct lsh_file_lock *i = (struct lsh_file_lock *) o;
  mark((struct lsh_object *) i->info);
}
struct lsh_class lsh_file_lock_class =
{
  STATIC_HEADER,
  &(resource_class),
  "lsh_file_lock",
  sizeof(struct lsh_file_lock),
  do_lsh_file_lock_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

