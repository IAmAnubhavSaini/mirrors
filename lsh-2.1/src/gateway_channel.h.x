/*
CLASS:gateway_channel:ssh_channel
*/
#ifndef GABA_DEFINE
struct gateway_channel
{
  struct ssh_channel super;
  struct gateway_channel *chain;
};
extern struct lsh_class gateway_channel_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_gateway_channel_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct gateway_channel *i = (struct gateway_channel *) o;
  mark((struct lsh_object *) i->chain);
}
struct lsh_class gateway_channel_class =
{
  STATIC_HEADER,
  &(ssh_channel_class),
  "gateway_channel",
  sizeof(struct gateway_channel),
  do_gateway_channel_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

