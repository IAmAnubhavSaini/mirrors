/*
CLASS:service_handler:packet_handler
*/
#ifndef GABA_DEFINE
struct service_handler
{
  struct packet_handler super;
  struct alist *services;
  struct command_continuation *c;
  struct exception_handler *e;
};
extern struct lsh_class service_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_service_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct service_handler *i = (struct service_handler *) o;
  mark((struct lsh_object *) i->services);
  mark((struct lsh_object *) i->c);
  mark((struct lsh_object *) i->e);
}
struct lsh_class service_handler_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "service_handler",
  sizeof(struct service_handler),
  do_service_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:offer_service:command
*/
#ifndef GABA_DEFINE
struct offer_service
{
  struct command super;
  struct alist *services;
};
extern struct lsh_class offer_service_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_offer_service_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct offer_service *i = (struct offer_service *) o;
  mark((struct lsh_object *) i->services);
}
struct lsh_class offer_service_class =
{
  STATIC_HEADER,
  &(command_class),
  "offer_service",
  sizeof(struct offer_service),
  do_offer_service_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

