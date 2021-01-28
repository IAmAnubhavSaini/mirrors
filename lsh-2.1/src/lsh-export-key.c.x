/*
CLASS:export_key_options:
*/
#ifndef GABA_DEFINE
struct export_key_options
{
  struct lsh_object super;
  struct alist *algorithms;
  enum output_mode mode;
  const char * infile;
  const char * outfile;
  const char * subject;
  const char * comment;
};
extern struct lsh_class export_key_options_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_export_key_options_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct export_key_options *i = (struct export_key_options *) o;
  mark((struct lsh_object *) i->algorithms);
}
struct lsh_class export_key_options_class =
{
  STATIC_HEADER,
  NULL,
  "export_key_options",
  sizeof(struct export_key_options),
  do_export_key_options_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

