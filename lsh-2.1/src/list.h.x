/*
CLASS:list_header:
*/
#ifndef GABA_DEFINE
struct list_header
{
  struct lsh_object super;
  unsigned length;
};
extern struct lsh_class list_header_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class list_header_class =
{
  STATIC_HEADER,
  NULL,
  "list_header",
  sizeof(struct list_header),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:int_list:list_header
*/
#ifndef GABA_DEFINE
struct int_list
{
  struct list_header super;
  int ((elements)[1]);
};
extern struct lsh_class int_list_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class int_list_class =
{
  STATIC_HEADER,
  &(list_header_class),
  "int_list",
  sizeof(struct int_list),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:object_list:list_header
*/
#ifndef GABA_DEFINE
struct object_list
{
  struct list_header super;
  struct lsh_object *((elements)[1]);
};
extern struct lsh_class object_list_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_object_list_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct object_list *i = (struct object_list *) o;
  {
    unsigned k3;
    for(k3=0; k3<i->super.length; k3++)
      mark((struct lsh_object *) (i->elements)[k3]);
  };
}
struct lsh_class object_list_class =
{
  STATIC_HEADER,
  &(list_header_class),
  "object_list",
  sizeof(struct object_list),
  do_object_list_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

