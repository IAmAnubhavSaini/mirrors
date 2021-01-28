/*
CLASS:terminal_attributes:
*/
#ifndef GABA_DEFINE
struct terminal_attributes
{
  struct lsh_object super;
  struct terminal_attributes *(*(make_raw))(struct terminal_attributes *self);
  struct lsh_string *(*(encode))(struct terminal_attributes *self);
};
extern struct lsh_class terminal_attributes_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class terminal_attributes_class =
{
  STATIC_HEADER,
  NULL,
  "terminal_attributes",
  sizeof(struct terminal_attributes),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:window_change_callback:
*/
#ifndef GABA_DEFINE
struct window_change_callback
{
  struct lsh_object super;
  void (*(f))(struct window_change_callback *self,struct interact *i);
};
extern struct lsh_class window_change_callback_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class window_change_callback_class =
{
  STATIC_HEADER,
  NULL,
  "window_change_callback",
  sizeof(struct window_change_callback),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:interact_dialog:
*/
#ifndef GABA_DEFINE
struct interact_dialog
{
  struct lsh_object super;
  struct lsh_string *instruction;
  unsigned nprompt;
  struct lsh_string *(*(prompt));
  struct lsh_string *(*(response));
  int (*(echo));
};
extern struct lsh_class interact_dialog_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_interact_dialog_free(struct lsh_object *o)
{
  struct interact_dialog *i = (struct interact_dialog *) o;
  lsh_string_free(i->instruction);
  {
    unsigned k4;
    for(k4=0; k4<i->nprompt; k4++)
      lsh_string_free((i->prompt)[k4]);
    lsh_space_free(i->prompt);
  };
  {
    unsigned k5;
    for(k5=0; k5<i->nprompt; k5++)
      lsh_string_free((i->response)[k5]);
    lsh_space_free(i->response);
  };
  lsh_space_free(i->echo);
}
struct lsh_class interact_dialog_class =
{
  STATIC_HEADER,
  NULL,
  "interact_dialog",
  sizeof(struct interact_dialog),
  NULL,
  do_interact_dialog_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:interact:
*/
#ifndef GABA_DEFINE
struct interact
{
  struct lsh_object super;
  int (*(is_tty))(struct interact *self);
  struct lsh_string *(*(read_password))(struct interact *self,uint32_t max_length,const struct lsh_string *prompt);
  void (*(set_askpass))(struct interact *self,const char *askpass);
  int (*(yes_or_no))(struct interact *self,const struct lsh_string *prompt,int def,int free);
  int (*(dialog))(struct interact *self,const struct interact_dialog *dialog);
  struct terminal_attributes *(*(get_attributes))(struct interact *self);
  int (*(set_attributes))(struct interact *self,struct terminal_attributes *attr);
  int (*(window_size))(struct interact *self,struct terminal_dimensions *);
  struct resource *(*(window_change_subscribe))(struct interact *self,struct window_change_callback *callback);
};
extern struct lsh_class interact_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class interact_class =
{
  STATIC_HEADER,
  NULL,
  "interact",
  sizeof(struct interact),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

