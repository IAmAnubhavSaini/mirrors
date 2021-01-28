/*
CLASS:lsh_file_lock_info:
*/
#ifndef GABA_DEFINE
struct lsh_file_lock_info
{
  struct lsh_object super;
  struct lsh_string *lockname;
  struct resource * (*(lock))(struct lsh_file_lock_info *self,unsigned retries);
  int (*(lock_p))(struct lsh_file_lock_info *self);
};
extern struct lsh_class lsh_file_lock_info_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_file_lock_info_free(struct lsh_object *o)
{
  struct lsh_file_lock_info *i = (struct lsh_file_lock_info *) o;
  lsh_string_free(i->lockname);
}
struct lsh_class lsh_file_lock_info_class =
{
  STATIC_HEADER,
  NULL,
  "lsh_file_lock_info",
  sizeof(struct lsh_file_lock_info),
  NULL,
  do_lsh_file_lock_info_free,
};
#endif /* !GABA_DECLARE */

