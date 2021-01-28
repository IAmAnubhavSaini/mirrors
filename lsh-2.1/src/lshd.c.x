/*
CLASS:lshd_options:algorithms_options
*/
#ifndef GABA_DEFINE
struct lshd_options
{
  struct algorithms_options super;
  struct reaper *reaper;
  struct randomness *random;
  struct alist *signature_algorithms;
  struct addr_queue local;
  char * port;
  char * hostkey;
  char * tcp_wrapper_name;
  char * tcp_wrapper_message;
  int with_srp_keyexchange;
  int with_dh_keyexchange;
  struct int_list *kex_algorithms;
  int with_loginauthmode;
  int with_publickey;
  int with_password;
  int allow_root;
  const char * pw_helper;
  const char * login_shell;
  const char * banner_file;
  int with_tcpip_forward;
  int with_x11_forward;
  int with_pty;
  const char ** subsystems;
  struct int_list *userauth_methods;
  struct alist *userauth_algorithms;
  int daemonic;
  int no_syslog;
  int corefile;
  const char * pid_file;
  int use_pid_file;
};
extern struct lsh_class lshd_options_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lshd_options_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct lshd_options *i = (struct lshd_options *) o;
  mark((struct lsh_object *) i->reaper);
  mark((struct lsh_object *) i->random);
  mark((struct lsh_object *) i->signature_algorithms);
  addr_queue_mark(&(i->local),
    mark);
  mark((struct lsh_object *) i->kex_algorithms);
  mark((struct lsh_object *) i->userauth_methods);
  mark((struct lsh_object *) i->userauth_algorithms);
}
static void
do_lshd_options_free(struct lsh_object *o)
{
  struct lshd_options *i = (struct lshd_options *) o;
  addr_queue_free(&(i->local));
}
struct lsh_class lshd_options_class =
{
  STATIC_HEADER,
  &(algorithms_options_class),
  "lshd_options",
  sizeof(struct lshd_options),
  do_lshd_options_mark,
  do_lshd_options_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:pid_file_resource:resource
*/
#ifndef GABA_DEFINE
struct pid_file_resource
{
  struct resource super;
  const char * file;
};
extern struct lsh_class pid_file_resource_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class pid_file_resource_class =
{
  STATIC_HEADER,
  &(resource_class),
  "pid_file_resource",
  sizeof(struct pid_file_resource),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:sighup_close_callback:lsh_callback
*/
#ifndef GABA_DEFINE
struct sighup_close_callback
{
  struct lsh_callback super;
  struct resource *resource;
};
extern struct lsh_class sighup_close_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_sighup_close_callback_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct sighup_close_callback *i = (struct sighup_close_callback *) o;
  mark((struct lsh_object *) i->resource);
}
struct lsh_class sighup_close_callback_class =
{
  STATIC_HEADER,
  &(lsh_callback_class),
  "sighup_close_callback",
  sizeof(struct sighup_close_callback),
  do_sighup_close_callback_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

static struct command *
lshd_connection_service(struct object_list *hooks)
  /* (B* (progn hooks) init_connection_service connection_require_userauth) */
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
MAKE_TRACE("lshd_connection_service",
    Bp3(A(PROGN,
        ((struct lsh_object *) hooks)),
      INIT_CONNECTION_SERVICE,
      CONNECTION_REQUIRE_USERAUTH)));
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
lshd_login_service(struct channel_open *handler)
  /* (C (B* install_session_handler init_connection_service connection_require_userauth) handler) */
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
MAKE_TRACE("lshd_login_service",
    C2(Bp3(INSTALL_SESSION_HANDLER,
        INIT_CONNECTION_SERVICE,
        CONNECTION_REQUIRE_USERAUTH),
      ((struct lsh_object *) handler))));
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
lshd_listen_callback(struct handshake_info *handshake,
  struct alist *keys,
  struct command *logger,
  struct command *services)
  /* (B* services (connection_handshake NULL handshake keys) logger) */
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
MAKE_TRACE("lshd_listen_callback",
    Bp3(((struct lsh_object *) services),
      A(A(A(CONNECTION_HANDSHAKE,
            NULL),
          ((struct lsh_object *) handshake)),
        ((struct lsh_object *) keys)),
      ((struct lsh_object *) logger))));
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
