/*
CLASS:srp_gen_options:
*/
#ifndef GABA_DEFINE
struct srp_gen_options
{
  struct lsh_object super;
  struct interact *tty;
  struct exception_handler *e;
  const struct zn_group *G;
  const struct hash_algorithm *H;
  struct lsh_string *file;
  struct abstract_write *dest;
  const char * name;
  struct lsh_string *passwd;
  struct randomness *r;
};
extern struct lsh_class srp_gen_options_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_srp_gen_options_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct srp_gen_options *i = (struct srp_gen_options *) o;
  mark((struct lsh_object *) i->tty);
  mark((struct lsh_object *) i->e);
  mark((struct lsh_object *) i->G);
  mark((struct lsh_object *) i->H);
  mark((struct lsh_object *) i->dest);
  mark((struct lsh_object *) i->r);
}
static void
do_srp_gen_options_free(struct lsh_object *o)
{
  struct srp_gen_options *i = (struct srp_gen_options *) o;
  lsh_string_free(i->file);
  lsh_string_free(i->passwd);
}
struct lsh_class srp_gen_options_class =
{
  STATIC_HEADER,
  NULL,
  "srp_gen_options",
  sizeof(struct srp_gen_options),
  do_srp_gen_options_mark,
  do_srp_gen_options_free,
};
#endif /* !GABA_DECLARE */

