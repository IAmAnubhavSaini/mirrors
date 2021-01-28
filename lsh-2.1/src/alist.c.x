/*
CLASS:alist_linear:alist:alist_meta
*/
#ifndef GABA_DEFINE
struct alist_linear
{
  struct alist super;
  struct lsh_object *((table)[NUMBER_OF_ATOMS]);
};
extern struct alist_meta alist_linear_class_extended;
#define alist_linear_class (alist_linear_class_extended.super)
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_alist_linear_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct alist_linear *i = (struct alist_linear *) o;
  {
    unsigned k1;
    for(k1=0; k1<NUMBER_OF_ATOMS; k1++)
      mark((struct lsh_object *) (i->table)[k1]);
  };
}
struct alist_meta alist_linear_class_extended =
{
  {
    STATIC_HEADER,
    &(alist_class),
    "alist_linear",
    sizeof(struct alist_linear),
    do_alist_linear_mark,
    NULL,
  },
  do_linear_get,
  do_linear_set,
};
#endif /* !GABA_DECLARE */

/*
CLASS:alist_linked:alist:alist_meta
*/
#ifndef GABA_DEFINE
struct alist_linked
{
  struct alist super;
  struct alist_node * head;
};
extern struct alist_meta alist_linked_class_extended;
#define alist_linked_class (alist_linked_class_extended.super)
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_alist_linked_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct alist_linked *i = (struct alist_linked *) o;
  do_mark_list(i->head,
    mark);
}
static void
do_alist_linked_free(struct lsh_object *o)
{
  struct alist_linked *i = (struct alist_linked *) o;
  do_free_list(i->head);
}
struct alist_meta alist_linked_class_extended =
{
  {
    STATIC_HEADER,
    &(alist_class),
    "alist_linked",
    sizeof(struct alist_linked),
    do_alist_linked_mark,
    do_alist_linked_free,
  },
  do_linked_get,
  do_linked_set,
};
#endif /* !GABA_DECLARE */

