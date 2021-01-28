/*
CLASS:lsh_writekey_options:
*/
#ifndef GABA_DEFINE
struct lsh_writekey_options
{
  struct lsh_object super;
  struct lsh_string *public_file;
  struct lsh_string *private_file;
  int server;
  struct interact *tty;
  struct lsh_string *label;
  struct lsh_string *passphrase;
  struct alist *crypto_algorithms;
  struct alist *signature_algorithms;
  struct randomness *r;
  int crypto_name;
  struct crypto_algorithm *crypto;
  uint32_t iterations;
};
extern struct lsh_class lsh_writekey_options_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_writekey_options_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct lsh_writekey_options *i = (struct lsh_writekey_options *) o;
  mark((struct lsh_object *) i->tty);
  mark((struct lsh_object *) i->crypto_algorithms);
  mark((struct lsh_object *) i->signature_algorithms);
  mark((struct lsh_object *) i->r);
  mark((struct lsh_object *) i->crypto);
}
static void
do_lsh_writekey_options_free(struct lsh_object *o)
{
  struct lsh_writekey_options *i = (struct lsh_writekey_options *) o;
  lsh_string_free(i->public_file);
  lsh_string_free(i->private_file);
  lsh_string_free(i->label);
  lsh_string_free(i->passphrase);
}
struct lsh_class lsh_writekey_options_class =
{
  STATIC_HEADER,
  NULL,
  "lsh_writekey_options",
  sizeof(struct lsh_writekey_options),
  do_lsh_writekey_options_mark,
  do_lsh_writekey_options_free,
};
#endif /* !GABA_DECLARE */

