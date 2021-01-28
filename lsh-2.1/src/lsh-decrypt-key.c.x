/*
CLASS:lsh_decryptkey_options:
*/
#ifndef GABA_DEFINE
struct lsh_decryptkey_options
{
  struct lsh_object super;
  struct interact *tty;
  int in_fd;
  int out_fd;
};
extern struct lsh_class lsh_decryptkey_options_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_decryptkey_options_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct lsh_decryptkey_options *i = (struct lsh_decryptkey_options *) o;
  mark((struct lsh_object *) i->tty);
}
struct lsh_class lsh_decryptkey_options_class =
{
  STATIC_HEADER,
  NULL,
  "lsh_decryptkey_options",
  sizeof(struct lsh_decryptkey_options),
  do_lsh_decryptkey_options_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

