/*
CLASS:dh_server_exchange:keyexchange_algorithm
*/
#ifndef GABA_DEFINE
struct dh_server_exchange
{
  struct keyexchange_algorithm super;
  struct dh_method *dh;
};
extern struct lsh_class dh_server_exchange_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_dh_server_exchange_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct dh_server_exchange *i = (struct dh_server_exchange *) o;
  mark((struct lsh_object *) i->dh);
}
struct lsh_class dh_server_exchange_class =
{
  STATIC_HEADER,
  &(keyexchange_algorithm_class),
  "dh_server_exchange",
  sizeof(struct dh_server_exchange),
  do_dh_server_exchange_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:dh_server:packet_handler
*/
#ifndef GABA_DEFINE
struct dh_server
{
  struct packet_handler super;
  struct dh_instance dh;
  int hostkey_algorithm;
  struct lsh_string *server_key;
  struct signer *signer;
  struct object_list *algorithms;
};
extern struct lsh_class dh_server_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_dh_server_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct dh_server *i = (struct dh_server *) o;
  dh_instance_mark(&(i->dh),
    mark);
  mark((struct lsh_object *) i->signer);
  mark((struct lsh_object *) i->algorithms);
}
static void
do_dh_server_free(struct lsh_object *o)
{
  struct dh_server *i = (struct dh_server *) o;
  dh_instance_free(&(i->dh));
  lsh_string_free(i->server_key);
}
struct lsh_class dh_server_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "dh_server",
  sizeof(struct dh_server),
  do_dh_server_mark,
  do_dh_server_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:srp_server_instance:
*/
#ifndef GABA_DEFINE
struct srp_server_instance
{
  struct lsh_object super;
  struct dh_instance dh;
  struct object_list *algorithms;
  struct user_db *db;
  struct lsh_user *user;
  struct srp_entry *entry;
};
extern struct lsh_class srp_server_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_srp_server_instance_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct srp_server_instance *i = (struct srp_server_instance *) o;
  dh_instance_mark(&(i->dh),
    mark);
  mark((struct lsh_object *) i->algorithms);
  mark((struct lsh_object *) i->db);
  mark((struct lsh_object *) i->user);
  mark((struct lsh_object *) i->entry);
}
static void
do_srp_server_instance_free(struct lsh_object *o)
{
  struct srp_server_instance *i = (struct srp_server_instance *) o;
  dh_instance_free(&(i->dh));
}
struct lsh_class srp_server_instance_class =
{
  STATIC_HEADER,
  NULL,
  "srp_server_instance",
  sizeof(struct srp_server_instance),
  do_srp_server_instance_mark,
  do_srp_server_instance_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:srp_server_handler:packet_handler
*/
#ifndef GABA_DEFINE
struct srp_server_handler
{
  struct packet_handler super;
  struct srp_server_instance *srp;
};
extern struct lsh_class srp_server_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_srp_server_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct srp_server_handler *i = (struct srp_server_handler *) o;
  mark((struct lsh_object *) i->srp);
}
struct lsh_class srp_server_handler_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "srp_server_handler",
  sizeof(struct srp_server_handler),
  do_srp_server_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:server_srp_read_verifier:abstract_write
*/
#ifndef GABA_DEFINE
struct server_srp_read_verifier
{
  struct abstract_write super;
  struct srp_server_instance *srp;
  struct ssh_connection *connection;
};
extern struct lsh_class server_srp_read_verifier_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_server_srp_read_verifier_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct server_srp_read_verifier *i = (struct server_srp_read_verifier *) o;
  mark((struct lsh_object *) i->srp);
  mark((struct lsh_object *) i->connection);
}
struct lsh_class server_srp_read_verifier_class =
{
  STATIC_HEADER,
  &(abstract_write_class),
  "server_srp_read_verifier",
  sizeof(struct server_srp_read_verifier),
  do_server_srp_read_verifier_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:srp_server_exchange:keyexchange_algorithm
*/
#ifndef GABA_DEFINE
struct srp_server_exchange
{
  struct keyexchange_algorithm super;
  const struct dh_method *dh;
  struct user_db *db;
};
extern struct lsh_class srp_server_exchange_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_srp_server_exchange_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct srp_server_exchange *i = (struct srp_server_exchange *) o;
  mark((struct lsh_object *) i->dh);
  mark((struct lsh_object *) i->db);
}
struct lsh_class srp_server_exchange_class =
{
  STATIC_HEADER,
  &(keyexchange_algorithm_class),
  "srp_server_exchange",
  sizeof(struct srp_server_exchange),
  do_srp_server_exchange_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

