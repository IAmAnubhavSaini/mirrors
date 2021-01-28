/*
CLASS:resource:
*/
#ifndef GABA_DEFINE
struct resource
{
  struct lsh_object super;
  int alive;
  void (*(kill))(struct resource *self);
};
extern struct lsh_class resource_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_resource_free(struct lsh_object *o)
{
  struct resource *i = (struct resource *) o;
  dont_free_live_resource(i->alive);
}
struct lsh_class resource_class =
{
  STATIC_HEADER,
  NULL,
  "resource",
  sizeof(struct resource),
  NULL,
  do_resource_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:resource_list:resource
*/
#ifndef GABA_DEFINE
struct resource_list
{
  struct resource super;
  struct resource_node * q;
};
extern struct lsh_class resource_list_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_resource_list_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct resource_list *i = (struct resource_list *) o;
  do_mark_resources(&(i->q),
    mark);
}
static void
do_resource_list_free(struct lsh_object *o)
{
  struct resource_list *i = (struct resource_list *) o;
  do_free_resources(&(i->q));
}
struct lsh_class resource_list_class =
{
  STATIC_HEADER,
  &(resource_class),
  "resource_list",
  sizeof(struct resource_list),
  do_resource_list_mark,
  do_resource_list_free,
};
#endif /* !GABA_DECLARE */

