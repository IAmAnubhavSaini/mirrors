/*
CLASS:kexinit_handler:packet_handler
*/
#ifndef GABA_DEFINE
struct kexinit_handler
{
  struct packet_handler super;
  struct lsh_object *extra;
  struct alist *algorithms;
};
extern struct lsh_class kexinit_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_kexinit_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct kexinit_handler *i = (struct kexinit_handler *) o;
  mark((struct lsh_object *) i->extra);
  mark((struct lsh_object *) i->algorithms);
}
struct lsh_class kexinit_handler_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "kexinit_handler",
  sizeof(struct kexinit_handler),
  do_kexinit_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:reexchange_timeout:lsh_callback
*/
#ifndef GABA_DEFINE
struct reexchange_timeout
{
  struct lsh_callback super;
  struct ssh_connection *connection;
};
extern struct lsh_class reexchange_timeout_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_reexchange_timeout_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct reexchange_timeout *i = (struct reexchange_timeout *) o;
  mark((struct lsh_object *) i->connection);
}
struct lsh_class reexchange_timeout_class =
{
  STATIC_HEADER,
  &(lsh_callback_class),
  "reexchange_timeout",
  sizeof(struct reexchange_timeout),
  do_reexchange_timeout_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:newkeys_handler:packet_handler
*/
#ifndef GABA_DEFINE
struct newkeys_handler
{
  struct packet_handler super;
  struct crypto_instance *crypto;
  struct mac_instance *mac;
  struct compress_instance *compression;
};
extern struct lsh_class newkeys_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_newkeys_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct newkeys_handler *i = (struct newkeys_handler *) o;
  mark((struct lsh_object *) i->crypto);
  mark((struct lsh_object *) i->mac);
  mark((struct lsh_object *) i->compression);
}
struct lsh_class newkeys_handler_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "newkeys_handler",
  sizeof(struct newkeys_handler),
  do_newkeys_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:simple_kexinit:make_kexinit
*/
#ifndef GABA_DEFINE
struct simple_kexinit
{
  struct make_kexinit super;
  struct randomness *r;
  struct int_list *kex_algorithms;
  struct int_list *hostkey_algorithms;
  struct int_list *crypto_algorithms;
  struct int_list *mac_algorithms;
  struct int_list *compression_algorithms;
  struct int_list *languages;
};
extern struct lsh_class simple_kexinit_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_simple_kexinit_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct simple_kexinit *i = (struct simple_kexinit *) o;
  mark((struct lsh_object *) i->r);
  mark((struct lsh_object *) i->kex_algorithms);
  mark((struct lsh_object *) i->hostkey_algorithms);
  mark((struct lsh_object *) i->crypto_algorithms);
  mark((struct lsh_object *) i->mac_algorithms);
  mark((struct lsh_object *) i->compression_algorithms);
  mark((struct lsh_object *) i->languages);
}
struct lsh_class simple_kexinit_class =
{
  STATIC_HEADER,
  &(make_kexinit_class),
  "simple_kexinit",
  sizeof(struct simple_kexinit),
  do_simple_kexinit_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

