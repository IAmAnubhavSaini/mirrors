/*
CLASS:accept_service_handler:packet_handler
*/
#ifndef GABA_DEFINE
struct accept_service_handler
{
  struct packet_handler super;
  int service;
  struct command_continuation *c;
};
extern struct lsh_class accept_service_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_accept_service_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct accept_service_handler *i = (struct accept_service_handler *) o;
  mark((struct lsh_object *) i->c);
}
struct lsh_class accept_service_handler_class =
{
  STATIC_HEADER,
  &(packet_handler_class),
  "accept_service_handler",
  sizeof(struct accept_service_handler),
  do_accept_service_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:detach_callback:lsh_callback
*/
#ifndef GABA_DEFINE
struct detach_callback
{
  struct lsh_callback super;
  int channel_flag;
  int fd_flag;
  int * exit_status;
};
extern struct lsh_class detach_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class detach_callback_class =
{
  STATIC_HEADER,
  &(lsh_callback_class),
  "detach_callback",
  sizeof(struct detach_callback),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:detach_resource:resource
*/
#ifndef GABA_DEFINE
struct detach_resource
{
  struct resource super;
  struct detach_callback *c;
};
extern struct lsh_class detach_resource_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_detach_resource_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct detach_resource *i = (struct detach_resource *) o;
  mark((struct lsh_object *) i->c);
}
struct lsh_class detach_resource_class =
{
  STATIC_HEADER,
  &(resource_class),
  "detach_resource",
  sizeof(struct detach_resource),
  do_detach_resource_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:exit_handler:channel_request
*/
#ifndef GABA_DEFINE
struct exit_handler
{
  struct channel_request super;
  int * exit_status;
};
extern struct lsh_class exit_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class exit_handler_class =
{
  STATIC_HEADER,
  &(channel_request_class),
  "exit_handler",
  sizeof(struct exit_handler),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:session_open_command:channel_open_command
*/
#ifndef GABA_DEFINE
struct session_open_command
{
  struct channel_open_command super;
  struct ssh_channel *session;
};
extern struct lsh_class session_open_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_session_open_command_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct session_open_command *i = (struct session_open_command *) o;
  mark((struct lsh_object *) i->session);
}
struct lsh_class session_open_command_class =
{
  STATIC_HEADER,
  &(channel_open_command_class),
  "session_open_command",
  sizeof(struct session_open_command),
  do_session_open_command_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:exec_request:channel_request_command
*/
#ifndef GABA_DEFINE
struct exec_request
{
  struct channel_request_command super;
  struct lsh_string *command;
};
extern struct lsh_class exec_request_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_exec_request_free(struct lsh_object *o)
{
  struct exec_request *i = (struct exec_request *) o;
  lsh_string_free(i->command);
}
struct lsh_class exec_request_class =
{
  STATIC_HEADER,
  &(channel_request_command_class),
  "exec_request",
  sizeof(struct exec_request),
  NULL,
  do_exec_request_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:subsystem_request:channel_request_command
*/
#ifndef GABA_DEFINE
struct subsystem_request
{
  struct channel_request_command super;
  struct lsh_string *subsystem;
};
extern struct lsh_class subsystem_request_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_subsystem_request_free(struct lsh_object *o)
{
  struct subsystem_request *i = (struct subsystem_request *) o;
  lsh_string_free(i->subsystem);
}
struct lsh_class subsystem_request_class =
{
  STATIC_HEADER,
  &(channel_request_command_class),
  "subsystem_request",
  sizeof(struct subsystem_request),
  NULL,
  do_subsystem_request_free,
};
#endif /* !GABA_DECLARE */

static struct command *
make_start_session(struct command *open_session,
  struct object_list *requests)
  /* (B (progn requests) open_session) */
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
MAKE_TRACE("make_start_session",
    B2(A(PROGN,
        ((struct lsh_object *) requests)),
      ((struct lsh_object *) open_session))));
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
client_start_session(struct command *request)
  /* (B client_start_io request) */
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
MAKE_TRACE("client_start_session",
    B2(CLIENT_START_IO,
      ((struct lsh_object *) request))));
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
CLASS:background_process_command:command
*/
#ifndef GABA_DEFINE
struct background_process_command
{
  struct command super;
  int write_pid;
};
extern struct lsh_class background_process_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class background_process_command_class =
{
  STATIC_HEADER,
  &(command_class),
  "background_process_command",
  sizeof(struct background_process_command),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

