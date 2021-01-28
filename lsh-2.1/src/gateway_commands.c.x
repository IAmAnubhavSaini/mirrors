/*
CLASS:read_gateway_packet:read_handler
*/
#ifndef GABA_DEFINE
struct read_gateway_packet
{
  struct read_handler super;
  uint32_t pos;
  struct lsh_string *header;
  struct lsh_string *payload;
  struct abstract_write *handler;
  struct ssh_connection *connection;
};
extern struct lsh_class read_gateway_packet_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_read_gateway_packet_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct read_gateway_packet *i = (struct read_gateway_packet *) o;
  mark((struct lsh_object *) i->handler);
  mark((struct lsh_object *) i->connection);
}
static void
do_read_gateway_packet_free(struct lsh_object *o)
{
  struct read_gateway_packet *i = (struct read_gateway_packet *) o;
  lsh_string_free(i->header);
  lsh_string_free(i->payload);
}
struct lsh_class read_gateway_packet_class =
{
  STATIC_HEADER,
  &(read_handler_class),
  "read_gateway_packet",
  sizeof(struct read_gateway_packet),
  do_read_gateway_packet_mark,
  do_read_gateway_packet_free,
};
#endif /* !GABA_DECLARE */

static struct command *
gateway_setup(struct local_info *local)
  /* (S connection_remember (S (B listen gateway_accept) (B bind_local (prog1 local)))) */
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
MAKE_TRACE("gateway_setup",
    S2(CONNECTION_REMEMBER,
      S2(B2(LISTEN,
          GATEWAY_ACCEPT),
        B2(BIND_LOCAL,
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
