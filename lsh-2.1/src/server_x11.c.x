/*
CLASS:channel_open_command_x11:channel_open_command
*/
#ifndef GABA_DEFINE
struct channel_open_command_x11
{
  struct channel_open_command super;
  struct listen_value *peer;
};
extern struct lsh_class channel_open_command_x11_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_channel_open_command_x11_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct channel_open_command_x11 *i = (struct channel_open_command_x11 *) o;
  mark((struct lsh_object *) i->peer);
}
struct lsh_class channel_open_command_x11_class =
{
  STATIC_HEADER,
  &(channel_open_command_class),
  "channel_open_command_x11",
  sizeof(struct channel_open_command_x11),
  do_channel_open_command_x11_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

static struct command *
server_x11_callback(struct ssh_connection *connection)
  /* (B start_io (C* catch_channel_open open_forwarded_x11 connection)) */
#define A GABA_APPLY
#define I GABA_VALUE_I
#define K GABA_VALUE_K
#define K1 GABA_APPLY_K_1
#define S GABA_VALUE_S
#define S1 GABA_APPLY_S_1
#define S2 GABA_APPLY_S_2
#define B GABA_VALUE_B
#define B1 GABA_APPLY_B_1
#define B2 GABA_APPLY_B_2
#define C GABA_VALUE_C
#define C1 GABA_APPLY_C_1
#define C2 GABA_APPLY_C_2
#define Sp GABA_VALUE_Sp
#define Sp1 GABA_APPLY_Sp_1
#define Sp2 GABA_APPLY_Sp_2
#define Sp3 GABA_APPLY_Sp_3
#define Bp GABA_VALUE_Bp
#define Bp1 GABA_APPLY_Bp_1
#define Bp2 GABA_APPLY_Bp_2
#define Bp3 GABA_APPLY_Bp_3
#define Cp GABA_VALUE_Cp
#define Cp1 GABA_APPLY_Cp_1
#define Cp2 GABA_APPLY_Cp_2
#define Cp3 GABA_APPLY_Cp_3
{
  CAST_SUBTYPE(command, res,
MAKE_TRACE("server_x11_callback",
    B2(START_IO,
      Cp3(CATCH_CHANNEL_OPEN,
        OPEN_FORWARDED_X11,
        ((struct lsh_object *) connection)))));
return res;
;
}
#undef A
#undef I
#undef K
#undef K1
#undef S
#undef S1
#undef S2
#undef B
#undef B1
#undef B2
#undef C
#undef C1
#undef C2
#undef Sp
#undef Sp1
#undef Sp2
#undef Sp3
#undef Bp
#undef Bp1
#undef Bp2
#undef Bp3
#undef Cp
#undef Cp1
#undef Cp2
#undef Cp3
/*
CLASS:server_x11_socket:resource
*/
#ifndef GABA_DEFINE
struct server_x11_socket
{
  struct resource super;
  int dir;
  struct lsh_string *name;
  int display_number;
  uid_t uid;
  struct lsh_fd *fd;
};
extern struct lsh_class server_x11_socket_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_server_x11_socket_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct server_x11_socket *i = (struct server_x11_socket *) o;
  mark((struct lsh_object *) i->fd);
}
static void
do_server_x11_socket_free(struct lsh_object *o)
{
  struct server_x11_socket *i = (struct server_x11_socket *) o;
  lsh_string_free(i->name);
}
struct lsh_class server_x11_socket_class =
{
  STATIC_HEADER,
  &(resource_class),
  "server_x11_socket",
  sizeof(struct server_x11_socket),
  do_server_x11_socket_mark,
  do_server_x11_socket_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:xauth_exit_callback:exit_callback
*/
#ifndef GABA_DEFINE
struct xauth_exit_callback
{
  struct exit_callback super;
  struct command_continuation *c;
  struct exception_handler *e;
};
extern struct lsh_class xauth_exit_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_xauth_exit_callback_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct xauth_exit_callback *i = (struct xauth_exit_callback *) o;
  mark((struct lsh_object *) i->c);
  mark((struct lsh_object *) i->e);
}
struct lsh_class xauth_exit_callback_class =
{
  STATIC_HEADER,
  &(exit_callback_class),
  "xauth_exit_callback",
  sizeof(struct xauth_exit_callback),
  do_xauth_exit_callback_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

