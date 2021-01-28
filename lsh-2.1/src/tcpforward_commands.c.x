/*
CLASS:open_tcpip_command:channel_open_command
*/
#ifndef GABA_DEFINE
struct open_tcpip_command
{
  struct channel_open_command super;
  int type;
  struct address_info *port;
  struct listen_value *peer;
};
extern struct lsh_class open_tcpip_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_open_tcpip_command_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct open_tcpip_command *i = (struct open_tcpip_command *) o;
  mark((struct lsh_object *) i->port);
  mark((struct lsh_object *) i->peer);
}
struct lsh_class open_tcpip_command_class =
{
  STATIC_HEADER,
  &(channel_open_command_class),
  "open_tcpip_command",
  sizeof(struct open_tcpip_command),
  do_open_tcpip_command_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:remote_port_install_continuation:command_frame
*/
#ifndef GABA_DEFINE
struct remote_port_install_continuation
{
  struct command_frame super;
  struct remote_port *port;
  struct command *callback;
};
extern struct lsh_class remote_port_install_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_remote_port_install_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct remote_port_install_continuation *i = (struct remote_port_install_continuation *) o;
  mark((struct lsh_object *) i->port);
  mark((struct lsh_object *) i->callback);
}
struct lsh_class remote_port_install_continuation_class =
{
  STATIC_HEADER,
  &(command_frame_class),
  "remote_port_install_continuation",
  sizeof(struct remote_port_install_continuation),
  do_remote_port_install_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:request_tcpip_forward_command:global_request_command
*/
#ifndef GABA_DEFINE
struct request_tcpip_forward_command
{
  struct global_request_command super;
  struct command *callback;
  struct address_info *port;
};
extern struct lsh_class request_tcpip_forward_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_request_tcpip_forward_command_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct request_tcpip_forward_command *i = (struct request_tcpip_forward_command *) o;
  mark((struct lsh_object *) i->callback);
  mark((struct lsh_object *) i->port);
}
struct lsh_class request_tcpip_forward_command_class =
{
  STATIC_HEADER,
  &(global_request_command_class),
  "request_tcpip_forward_command",
  sizeof(struct request_tcpip_forward_command),
  do_request_tcpip_forward_command_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

static struct command *
forward_local_port(struct address_info *local,
  struct address_info *target)
  /* (S connection_remember (S (B* listen (B start_io) (C* catch_channel_open (open_direct_tcpip target))) (B bind (prog1 local)))) */
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
MAKE_TRACE("forward_local_port",
    S2(CONNECTION_REMEMBER,
      S2(Bp3(LISTEN,
          B1(START_IO),
          Cp2(CATCH_CHANNEL_OPEN,
            A(OPEN_DIRECT_TCPIP,
              ((struct lsh_object *) target)))),
        B2(BIND,
          A(PROG1,
            ((struct lsh_object *) local)))))));
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
static struct command *
forward_remote_port(struct command *connect,
  struct address_info *remote)
  /* (S (C (B* remote_listen (B* tcpip_connect_io connect) prog1) remote) I) */
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
MAKE_TRACE("forward_remote_port",
    S2(C2(Bp3(REMOTE_LISTEN,
          Bp2(TCPIP_CONNECT_IO,
            ((struct lsh_object *) connect)),
          PROG1),
        ((struct lsh_object *) remote)),
      I)));
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
static struct command *
direct_tcpip_hook(void)
  /* (S install_direct_tcpip (B* direct_tcpip_handler (B tcpip_connect_io) connect_connection)) */
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
MAKE_TRACE("direct_tcpip_hook",
    S2(INSTALL_DIRECT_TCPIP,
      Bp3(DIRECT_TCPIP_HANDLER,
        B1(TCPIP_CONNECT_IO),
        CONNECT_CONNECTION))));
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
static struct command *
tcpip_forward_hook(void)
  /* (S install_tcpip_forward (B tcpip_forward_handler (C (B* S (B* listen (B start_io)) (C* (C* catch_channel_open) open_forwarded_tcpip)) bind))) */
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
MAKE_TRACE("tcpip_forward_hook",
    S2(INSTALL_TCPIP_FORWARD,
      B2(TCPIP_FORWARD_HANDLER,
        C2(Bp3(S,
            Bp2(LISTEN,
              B1(START_IO)),
            Cp2(Cp1(CATCH_CHANNEL_OPEN),
              OPEN_FORWARDED_TCPIP)),
          BIND)))));
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
