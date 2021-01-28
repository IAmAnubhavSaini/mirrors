/*
CLASS:algorithms_options:
*/
#ifndef GABA_DEFINE
struct algorithms_options
{
  struct lsh_object super;
  struct alist *algorithms;
  struct int_list *crypto_algorithms;
  struct int_list *mac_algorithms;
  struct int_list *compression_algorithms;
  struct int_list *hostkey_algorithms;
};
extern struct lsh_class algorithms_options_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_algorithms_options_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct algorithms_options *i = (struct algorithms_options *) o;
  mark((struct lsh_object *) i->algorithms);
  mark((struct lsh_object *) i->crypto_algorithms);
  mark((struct lsh_object *) i->mac_algorithms);
  mark((struct lsh_object *) i->compression_algorithms);
  mark((struct lsh_object *) i->hostkey_algorithms);
}
struct lsh_class algorithms_options_class =
{
  STATIC_HEADER,
  NULL,
  "algorithms_options",
  sizeof(struct algorithms_options),
  do_algorithms_options_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

