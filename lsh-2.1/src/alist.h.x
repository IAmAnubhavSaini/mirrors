#ifndef GABA_DEFINE
struct alist_meta
{
  struct lsh_class super;
  struct lsh_object * (*get)(struct alist *self, int atom);
  void (*set)(struct alist *self, int atom, struct lsh_object *value);
};
#endif /* !GABA_DEFINE */
/*
CLASS:alist::alist_meta
*/
#ifndef GABA_DEFINE
struct alist
{
  struct lsh_object super;
  unsigned size;
};
extern struct alist_meta alist_class_extended;
#define alist_class (alist_class_extended.super)
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct alist_meta alist_class_extended =
{
  {
    STATIC_HEADER,
    NULL,
    "alist",
    sizeof(struct alist),
    NULL,
    NULL,
  },
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

