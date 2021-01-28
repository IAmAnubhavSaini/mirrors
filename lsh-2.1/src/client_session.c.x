/*
CLASS:client_session_channel:ssh_channel
*/
#ifndef GABA_DEFINE
struct client_session_channel
{
  struct ssh_channel super;
  struct lsh_fd *in;
  struct lsh_fd *out;
  struct lsh_fd *err;
  struct escape_info *escape;
  int * exit_status;
};
extern struct lsh_class client_session_channel_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_client_session_channel_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct client_session_channel *i = (struct client_session_channel *) o;
  mark((struct lsh_object *) i->in);
  mark((struct lsh_object *) i->out);
  mark((struct lsh_object *) i->err);
  mark((struct lsh_object *) i->escape);
}
struct lsh_class client_session_channel_class =
{
  STATIC_HEADER,
  &(ssh_channel_class),
  "client_session_channel",
  sizeof(struct client_session_channel),
  do_client_session_channel_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

