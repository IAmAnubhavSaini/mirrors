/*
CLASS:socks_connection:
*/
#ifndef GABA_DEFINE
struct socks_connection
{
  struct lsh_object super;
  uint8_t version;
  struct ssh_connection *connection;
  struct listen_value *peer;
};
extern struct lsh_class socks_connection_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_socks_connection_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct socks_connection *i = (struct socks_connection *) o;
  mark((struct lsh_object *) i->connection);
  mark((struct lsh_object *) i->peer);
}
struct lsh_class socks_connection_class =
{
  STATIC_HEADER,
  NULL,
  "socks_connection",
  sizeof(struct socks_connection),
  do_socks_connection_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:socks_continuation:command_continuation
*/
#ifndef GABA_DEFINE
struct socks_continuation
{
  struct command_continuation super;
  struct socks_connection *socks;
};
extern struct lsh_class socks_continuation_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_socks_continuation_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct socks_continuation *i = (struct socks_continuation *) o;
  mark((struct lsh_object *) i->socks);
}
struct lsh_class socks_continuation_class =
{
  STATIC_HEADER,
  &(command_continuation_class),
  "socks_continuation",
  sizeof(struct socks_continuation),
  do_socks_continuation_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:socks_exception_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct socks_exception_handler
{
  struct exception_handler super;
  struct socks_connection *socks;
};
extern struct lsh_class socks_exception_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_socks_exception_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct socks_exception_handler *i = (struct socks_exception_handler *) o;
  mark((struct lsh_object *) i->socks);
}
struct lsh_class socks_exception_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "socks_exception_handler",
  sizeof(struct socks_exception_handler),
  do_socks_exception_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:read_socks:read_handler
*/
#ifndef GABA_DEFINE
struct read_socks
{
  struct read_handler super;
  struct socks_connection *socks;
  struct lsh_string *buffer;
  uint32_t pos;
  enum socks_state state;
  uint32_t length;
};
extern struct lsh_class read_socks_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_read_socks_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct read_socks *i = (struct read_socks *) o;
  mark((struct lsh_object *) i->socks);
}
static void
do_read_socks_free(struct lsh_object *o)
{
  struct read_socks *i = (struct read_socks *) o;
  lsh_string_free(i->buffer);
}
struct lsh_class read_socks_class =
{
  STATIC_HEADER,
  &(read_handler_class),
  "read_socks",
  sizeof(struct read_socks),
  do_read_socks_mark,
  do_read_socks_free,
};
#endif /* !GABA_DECLARE */

static struct command *
forward_socks(struct address_info *local)
  /* (S connection_remember (S (B listen (C socks_handshake)) (B bind (prog1 local)))) */
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
MAKE_TRACE("forward_socks",
    S2(CONNECTION_REMEMBER,
      S2(B2(LISTEN,
          C1(SOCKS_HANDSHAKE)),
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
