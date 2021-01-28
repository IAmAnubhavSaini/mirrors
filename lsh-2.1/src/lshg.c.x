/*
CLASS:lshg_options:client_options
*/
#ifndef GABA_DEFINE
struct lshg_options
{
  struct client_options super;
  struct local_info *gateway;
  int fallback_lsh;
};
extern struct lsh_class lshg_options_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_lshg_options_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct lshg_options *i = (struct lshg_options *) o;
  mark((struct lsh_object *) i->gateway);
}
struct lsh_class lshg_options_class =
{
  STATIC_HEADER,
  &(client_options_class),
  "lshg_options",
  sizeof(struct lshg_options),
  do_lshg_options_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

static struct command *
make_lshg_connect(struct resource *resource,
  struct object_list *actions)
  /* (B (progn actions) (B* (protect resource gateway_init) connect_local options2info)) */
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
MAKE_TRACE("make_lshg_connect",
    B2(A(PROGN,
        ((struct lsh_object *) actions)),
      Bp3(A(A(PROTECT,
            ((struct lsh_object *) resource)),
          GATEWAY_INIT),
        CONNECT_LOCAL,
        OPTIONS2INFO))));
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
CLASS:lshg_simple_action:command
*/
#ifndef GABA_DEFINE
struct lshg_simple_action
{
  struct command super;
  const char * msg;
};
extern struct lsh_class lshg_simple_action_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class lshg_simple_action_class =
{
  STATIC_HEADER,
  &(command_class),
  "lshg_simple_action",
  sizeof(struct lshg_simple_action),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:lshg_exception_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct lshg_exception_handler
{
  struct exception_handler super;
  char** argv;
  int fallback_lsh;
};
extern struct lsh_class lshg_exception_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class lshg_exception_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "lshg_exception_handler",
  sizeof(struct lshg_exception_handler),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

