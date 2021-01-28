/*
CLASS:handshake_info:
*/
#ifndef GABA_DEFINE
struct handshake_info
{
  struct lsh_object super;
  enum connection_flag flags;
  uint32_t block_size;
  const char * id_comment;
  const char * debug_comment;
  struct randomness *random;
  struct alist *algorithms;
  struct make_kexinit *kexinit;
  struct lsh_string *banner_text;
};
extern struct lsh_class handshake_info_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_handshake_info_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct handshake_info *i = (struct handshake_info *) o;
  mark((struct lsh_object *) i->random);
  mark((struct lsh_object *) i->algorithms);
  mark((struct lsh_object *) i->kexinit);
}
static void
do_handshake_info_free(struct lsh_object *o)
{
  struct handshake_info *i = (struct handshake_info *) o;
  lsh_string_free(i->banner_text);
}
struct lsh_class handshake_info_class =
{
  STATIC_HEADER,
  NULL,
  "handshake_info",
  sizeof(struct handshake_info),
  do_handshake_info_mark,
  do_handshake_info_free,
};
#endif /* !GABA_DECLARE */

