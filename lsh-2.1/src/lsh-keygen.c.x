/*
CLASS:lsh_keygen_options:
*/
#ifndef GABA_DEFINE
struct lsh_keygen_options
{
  struct lsh_object super;
  int server;
  int algorithm;
  int level;
};
extern struct lsh_class lsh_keygen_options_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class lsh_keygen_options_class =
{
  STATIC_HEADER,
  NULL,
  "lsh_keygen_options",
  sizeof(struct lsh_keygen_options),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

