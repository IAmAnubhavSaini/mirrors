/*
CLASS:server_x11_info:
*/
#ifndef GABA_DEFINE
struct server_x11_info
{
  struct lsh_object super;
  struct lsh_string *display;
  struct lsh_string *xauthority;
};
extern struct lsh_class server_x11_info_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_server_x11_info_free(struct lsh_object *o)
{
  struct server_x11_info *i = (struct server_x11_info *) o;
  lsh_string_free(i->display);
  lsh_string_free(i->xauthority);
}
struct lsh_class server_x11_info_class =
{
  STATIC_HEADER,
  NULL,
  "server_x11_info",
  sizeof(struct server_x11_info),
  NULL,
  do_server_x11_info_free,
};
#endif /* !GABA_DECLARE */

