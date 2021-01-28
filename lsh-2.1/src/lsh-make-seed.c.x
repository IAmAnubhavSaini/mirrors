/*
CLASS:lsh_make_seed_options:
*/
#ifndef GABA_DEFINE
struct lsh_make_seed_options
{
  struct lsh_object super;
  struct lsh_string *directory;
  struct lsh_string *filename;
  int force;
  int sloppy;
};
extern struct lsh_class lsh_make_seed_options_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_make_seed_options_free(struct lsh_object *o)
{
  struct lsh_make_seed_options *i = (struct lsh_make_seed_options *) o;
  lsh_string_free(i->directory);
  lsh_string_free(i->filename);
}
struct lsh_class lsh_make_seed_options_class =
{
  STATIC_HEADER,
  NULL,
  "lsh_make_seed_options",
  sizeof(struct lsh_make_seed_options),
  NULL,
  do_lsh_make_seed_options_free,
};
#endif /* !GABA_DECLARE */

