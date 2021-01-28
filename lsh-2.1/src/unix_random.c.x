/*
CLASS:unix_random:randomness
*/
#ifndef GABA_DEFINE
struct unix_random
{
  struct randomness super;
  int seed_file_fd;
  struct lsh_file_lock_info *lock;
  struct yarrow256_ctx yarrow;
  struct yarrow_source ((sources)[RANDOM_NSOURCES]);
  long previous_time;
  unsigned time_count;
  int device_fd;
  time_t device_last_read;
};
extern struct lsh_class unix_random_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_unix_random_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct unix_random *i = (struct unix_random *) o;
  mark((struct lsh_object *) i->lock);
}
struct lsh_class unix_random_class =
{
  STATIC_HEADER,
  &(randomness_class),
  "unix_random",
  sizeof(struct unix_random),
  do_unix_random_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

