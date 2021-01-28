/*
CLASS:packet_handler:
*/
#ifndef GABA_DEFINE
struct packet_handler
{
  struct lsh_object super;
  void (*(handler))(struct packet_handler *self,struct ssh_connection *connection,struct lsh_string *packet);
};
extern struct lsh_class packet_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class packet_handler_class =
{
  STATIC_HEADER,
  NULL,
  "packet_handler",
  sizeof(struct packet_handler),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:ssh_connection:abstract_write
*/
#ifndef GABA_DEFINE
struct ssh_connection
{
  struct abstract_write super;
  struct exception_handler *e;
  enum connection_flag flags;
  struct lsh_string *((versions)[2]);
  struct lsh_string *session_id;
  const char * debug_comment;
  enum peer_flag peer_flags;
  struct resource *timer;
  struct lsh_user *user;
  struct ssh_connection *chain;
  struct resource_list *resources;
  struct address_info *peer;
  struct address_info *local;
  struct make_kexinit *kexinit;
  uint32_t rec_max_packet;
  struct mac_instance *rec_mac;
  struct crypto_instance *rec_crypto;
  struct compress_instance *rec_compress;
  struct lsh_fd *socket;
  uint32_t soft_limit;
  uint32_t hard_limit;
  struct abstract_write *write_packet;
  struct mac_instance *send_mac;
  struct crypto_instance *send_crypto;
  struct compress_instance *send_compress;
  int paused;
  struct string_queue pending;
  enum kex_state read_kex_state;
  int send_kex_only;
  struct string_queue send_queue;
  struct resource *key_expire;
  uint32_t sent_data;
  struct command_continuation *keyexchange_done;
  struct command_continuation *wakeup;
  struct kexinit *((kexinits)[2]);
  struct lsh_string *((literal_kexinits)[2]);
  struct packet_handler *((dispatch)[0x100]);
  struct channel_table *table;
};
extern struct lsh_class ssh_connection_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_ssh_connection_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct ssh_connection *i = (struct ssh_connection *) o;
  mark((struct lsh_object *) i->e);
  mark((struct lsh_object *) i->timer);
  mark((struct lsh_object *) i->user);
  mark((struct lsh_object *) i->chain);
  mark((struct lsh_object *) i->resources);
  mark((struct lsh_object *) i->peer);
  mark((struct lsh_object *) i->local);
  mark((struct lsh_object *) i->kexinit);
  mark((struct lsh_object *) i->rec_mac);
  mark((struct lsh_object *) i->rec_crypto);
  mark((struct lsh_object *) i->rec_compress);
  mark((struct lsh_object *) i->socket);
  mark((struct lsh_object *) i->write_packet);
  mark((struct lsh_object *) i->send_mac);
  mark((struct lsh_object *) i->send_crypto);
  mark((struct lsh_object *) i->send_compress);
  string_queue_mark(&(i->pending),
    mark);
  string_queue_mark(&(i->send_queue),
    mark);
  mark((struct lsh_object *) i->key_expire);
  mark((struct lsh_object *) i->keyexchange_done);
  mark((struct lsh_object *) i->wakeup);
  {
    unsigned k2;
    for(k2=0; k2<2; k2++)
      mark((struct lsh_object *) (i->kexinits)[k2]);
  };
  {
    unsigned k4;
    for(k4=0; k4<0x100; k4++)
      mark((struct lsh_object *) (i->dispatch)[k4]);
  };
  mark((struct lsh_object *) i->table);
}
static void
do_ssh_connection_free(struct lsh_object *o)
{
  struct ssh_connection *i = (struct ssh_connection *) o;
  {
    unsigned k5;
    for(k5=0; k5<2; k5++)
      lsh_string_free((i->versions)[k5]);
  };
  lsh_string_free(i->session_id);
  string_queue_free(&(i->pending));
  string_queue_free(&(i->send_queue));
  {
    unsigned k7;
    for(k7=0; k7<2; k7++)
      lsh_string_free((i->literal_kexinits)[k7]);
  };
}
struct lsh_class ssh_connection_class =
{
  STATIC_HEADER,
  &(abstract_write_class),
  "ssh_connection",
  sizeof(struct ssh_connection),
  do_ssh_connection_mark,
  do_ssh_connection_free,
};
#endif /* !GABA_DECLARE */

