/*
CLASS:lsh_options:client_options
*/
#ifndef GABA_DEFINE
struct lsh_options
{
  struct client_options super;
  struct algorithms_options *algorithms;
  struct alist *signature_algorithms;
  const char * home;
  char * identity;
  int with_publickey;
  int with_srp_keyexchange;
  int with_dh_keyexchange;
  struct int_list *kex_algorithms;
  int sloppy;
  const char * capture;
  struct abstract_write *capture_file;
  const char * known_hosts;
  int start_gateway;
};
extern struct lsh_class lsh_options_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_options_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct lsh_options *i = (struct lsh_options *) o;
  mark((struct lsh_object *) i->algorithms);
  mark((struct lsh_object *) i->signature_algorithms);
  mark((struct lsh_object *) i->kex_algorithms);
  mark((struct lsh_object *) i->capture_file);
}
struct lsh_class lsh_options_class =
{
  STATIC_HEADER,
  &(client_options_class),
  "lsh_options",
  sizeof(struct lsh_options),
  do_lsh_options_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:lsh_host_db:lookup_verifier
*/
#ifndef GABA_DEFINE
struct lsh_host_db
{
  struct lookup_verifier super;
  struct spki_context *db;
  struct interact *tty;
  struct lsh_string *access;
  const char * host;
  int sloppy;
  struct abstract_write *file;
  const struct hash_algorithm *hash;
};
extern struct lsh_class lsh_host_db_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lsh_host_db_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct lsh_host_db *i = (struct lsh_host_db *) o;
  mark((struct lsh_object *) i->db);
  mark((struct lsh_object *) i->tty);
  mark((struct lsh_object *) i->file);
  mark((struct lsh_object *) i->hash);
}
static void
do_lsh_host_db_free(struct lsh_object *o)
{
  struct lsh_host_db *i = (struct lsh_host_db *) o;
  lsh_string_free(i->access);
}
struct lsh_class lsh_host_db_class =
{
  STATIC_HEADER,
  &(lookup_verifier_class),
  "lsh_host_db",
  sizeof(struct lsh_host_db),
  do_lsh_host_db_mark,
  do_lsh_host_db_free,
};
#endif /* !GABA_DECLARE */

static struct command *
make_lsh_connect(struct resource *resource,
  struct handshake_info *handshake,
  struct lookup_verifier *db,
  struct object_list *actions,
  struct command *login)
  /* (B (progn actions) (B* init_connection_service login (B* request_userauth_service (protect resource connection_handshake handshake db) connect_list))) */
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
MAKE_TRACE("make_lsh_connect",
    B2(A(PROGN,
        ((struct lsh_object *) actions)),
      Bp3(INIT_CONNECTION_SERVICE,
        ((struct lsh_object *) login),
        Bp3(REQUEST_USERAUTH_SERVICE,
          A(A(A(A(PROTECT,
                  ((struct lsh_object *) resource)),
                CONNECTION_HANDSHAKE),
              ((struct lsh_object *) handshake)),
            ((struct lsh_object *) db)),
          CONNECT_LIST)))));
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
CLASS:lsh_default_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct lsh_default_handler
{
  struct exception_handler super;
  int * status;
};
extern struct lsh_class lsh_default_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class lsh_default_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "lsh_default_handler",
  sizeof(struct lsh_default_handler),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

