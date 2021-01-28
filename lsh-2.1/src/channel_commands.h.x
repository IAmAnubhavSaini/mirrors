/*
CLASS:channel_open_command:command
*/
#ifndef GABA_DEFINE
struct channel_open_command
{
  struct command super;
  struct ssh_channel * (*(new_channel))(struct channel_open_command *self,struct ssh_connection *connection,uint32_t local_channel_number,struct lsh_string **request);
};
extern struct lsh_class channel_open_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class channel_open_command_class =
{
  STATIC_HEADER,
  &(command_class),
  "channel_open_command",
  sizeof(struct channel_open_command),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:channel_request_command:command
*/
#ifndef GABA_DEFINE
struct channel_request_command
{
  struct command super;
  struct lsh_string * (*(format_request))(struct channel_request_command *self,struct ssh_channel *channel,struct command_continuation **c);
};
extern struct lsh_class channel_request_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class channel_request_command_class =
{
  STATIC_HEADER,
  &(command_class),
  "channel_request_command",
  sizeof(struct channel_request_command),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:global_request_command:command
*/
#ifndef GABA_DEFINE
struct global_request_command
{
  struct command super;
  struct lsh_string * (*(format_request))(struct global_request_command *self,struct ssh_connection *connection,struct command_continuation **c);
};
extern struct lsh_class global_request_command_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class global_request_command_class =
{
  STATIC_HEADER,
  &(command_class),
  "global_request_command",
  sizeof(struct global_request_command),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:install_info:command_2
*/
#ifndef GABA_DEFINE
struct install_info
{
  struct command_2 super;
  int name;
};
extern struct lsh_class install_info_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class install_info_class =
{
  STATIC_HEADER,
  &(command_2_class),
  "install_info",
  sizeof(struct install_info),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

