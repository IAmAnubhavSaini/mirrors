/*
CLASS:lsh_decode_key_options:
*/
#ifndef GABA_DEFINE
struct lsh_decode_key_options
{
  struct lsh_object super;
  struct lsh_string *file;
  int base64;
};
extern struct lsh_class lsh_decode_key_options_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_decode_key_options_free(struct lsh_object *o)
{
  struct lsh_decode_key_options *i = (struct lsh_decode_key_options *) o;
  lsh_string_free(i->file);
}
struct lsh_class lsh_decode_key_options_class =
{
  STATIC_HEADER,
  NULL,
  "lsh_decode_key_options",
  sizeof(struct lsh_decode_key_options),
  NULL,
  do_lsh_decode_key_options_free,
};
#endif /* !GABA_DECLARE */

