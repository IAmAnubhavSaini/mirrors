/*
CLASS:dh_client_exchange:keyexchange_algorithm
*/
#ifndef GABA_DEFINE
struct dh_client_exchange
{
  struct keyexchange_algorithm super;
  const struct dh_method *dh;
};
extern struct lsh_class dh_client_exchange_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_dh_client_exchange_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct dh_client_exchange *i = (struct dh_client_exchange *) o;
  mark((struct lsh_object *) i->dh);
}
struct lsh_class dh_client_exchange_class =
{
  STATIC_HEADER,
  &(keyexchange_algorithm_class),
  "dh_client_exchange",
  sizeof(struct dh_client_exchange),
  do_dh_client_exchange_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:dh_client:packet_handler
*/
#ifndef GABA_DEFINE
struct dh_client
{
  struct packet_handler super;
  struct dh_instance dh;
  int hostkey_algorithm;
  struct lookup_verifier *verifier;
  struct object_list *algorithms;
};
extern struct lsh_class dh_client_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_dh_client_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct dh_client *i = (struct dh_client *) o;
  dh_instance_mark(&(i->dh),
    mark);
  mark((struct lsh_object *) i->verifier);
  mark((struct lsh_object *) i->algorithms);
}
static void
do_dh_client_free(struct lsh_object *o)
{
  struct dh_client *i = (struct dh_client *) o;
  dh_instance_free(&(i->dh));
}
struct lsh_class dh_client_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "dh_client",
  sizeof(struct dh_client),
  do_dh_client_mark,
  do_dh_client_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:srp_client_instance:
*/
#ifndef GABA_DEFINE
struct srp_client_instance
{
  struct lsh_object super;
  struct dh_instance dh;
  struct interact *tty;
  struct lsh_string *name;
  struct lsh_string *m2;
  struct object_list *algorithms;
};
extern struct lsh_class srp_client_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_srp_client_instance_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct srp_client_instance *i = (struct srp_client_instance *) o;
  dh_instance_mark(&(i->dh),
    mark);
  mark((struct lsh_object *) i->tty);
  mark((struct lsh_object *) i->algorithms);
}
static void
do_srp_client_instance_free(struct lsh_object *o)
{
  struct srp_client_instance *i = (struct srp_client_instance *) o;
  dh_instance_free(&(i->dh));
  lsh_string_free(i->name);
  lsh_string_free(i->m2);
}
struct lsh_class srp_client_instance_class =
{
  STATIC_HEADER,
  NULL,
  "srp_client_instance",
  sizeof(struct srp_client_instance),
  do_srp_client_instance_mark,
  do_srp_client_instance_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:srp_client_handler:packet_handler
*/
#ifndef GABA_DEFINE
struct srp_client_handler
{
  struct packet_handler super;
  struct srp_client_instance *srp;
};
extern struct lsh_class srp_client_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_srp_client_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct srp_client_handler *i = (struct srp_client_handler *) o;
  mark((struct lsh_object *) i->srp);
}
struct lsh_class srp_client_handler_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "srp_client_handler",
  sizeof(struct srp_client_handler),
  do_srp_client_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:srp_client_exchange:keyexchange_algorithm
*/
#ifndef GABA_DEFINE
struct srp_client_exchange
{
  struct keyexchange_algorithm super;
  const struct dh_method *dh;
  struct interact *tty;
  struct lsh_string *name;
};
extern struct lsh_class srp_client_exchange_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_srp_client_exchange_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct srp_client_exchange *i = (struct srp_client_exchange *) o;
  mark((struct lsh_object *) i->dh);
  mark((struct lsh_object *) i->tty);
}
static void
do_srp_client_exchange_free(struct lsh_object *o)
{
  struct srp_client_exchange *i = (struct srp_client_exchange *) o;
  lsh_string_free(i->name);
}
struct lsh_class srp_client_exchange_class =
{
  STATIC_HEADER,
  &(keyexchange_algorithm_class),
  "srp_client_exchange",
  sizeof(struct srp_client_exchange),
  do_srp_client_exchange_mark,
  do_srp_client_exchange_free,
};
#endif /* !GABA_DECLARE */

