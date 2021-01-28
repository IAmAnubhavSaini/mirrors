/*
CLASS:keyexchange_algorithm:
*/
#ifndef GABA_DEFINE
struct keyexchange_algorithm
{
  struct lsh_object super;
  void (*(init))(struct keyexchange_algorithm *self,struct ssh_connection *connection,int hostkey_algorithm_atom,struct lsh_object *extra,struct object_list *algorithms);
};
extern struct lsh_class keyexchange_algorithm_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class keyexchange_algorithm_class =
{
  STATIC_HEADER,
  NULL,
  "keyexchange_algorithm",
  sizeof(struct keyexchange_algorithm),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:kexinit:
*/
#ifndef GABA_DEFINE
struct kexinit
{
  struct lsh_object super;
  uint8_t ((cookie)[16]);
  struct int_list *kex_algorithms;
  struct int_list *server_hostkey_algorithms;
  struct int_list *((parameters)[KEX_PARAMETERS]);
  struct int_list *languages_client_to_server;
  struct int_list *languages_server_to_client;
  int first_kex_packet_follows;
  struct lsh_string *first_kex_packet;
};
extern struct lsh_class kexinit_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_kexinit_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct kexinit *i = (struct kexinit *) o;
  mark((struct lsh_object *) i->kex_algorithms);
  mark((struct lsh_object *) i->server_hostkey_algorithms);
  {
    unsigned k2;
    for(k2=0; k2<KEX_PARAMETERS; k2++)
      mark((struct lsh_object *) (i->parameters)[k2]);
  };
  mark((struct lsh_object *) i->languages_client_to_server);
  mark((struct lsh_object *) i->languages_server_to_client);
}
static void
do_kexinit_free(struct lsh_object *o)
{
  struct kexinit *i = (struct kexinit *) o;
  lsh_string_free(i->first_kex_packet);
}
struct lsh_class kexinit_class =
{
  STATIC_HEADER,
  NULL,
  "kexinit",
  sizeof(struct kexinit),
  do_kexinit_mark,
  do_kexinit_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:make_kexinit:
*/
#ifndef GABA_DEFINE
struct make_kexinit
{
  struct lsh_object super;
  struct kexinit *(*(make))(struct make_kexinit *self);
};
extern struct lsh_class make_kexinit_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class make_kexinit_class =
{
  STATIC_HEADER,
  NULL,
  "make_kexinit",
  sizeof(struct make_kexinit),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

