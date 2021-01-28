/*
CLASS:window_subscriber:resource
*/
#ifndef GABA_DEFINE
struct window_subscriber
{
  struct resource super;
  struct unix_interact *interact;
  struct window_subscriber *next;
  struct window_change_callback *callback;
};
extern struct lsh_class window_subscriber_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_window_subscriber_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct window_subscriber *i = (struct window_subscriber *) o;
  mark((struct lsh_object *) i->interact);
  mark((struct lsh_object *) i->next);
  mark((struct lsh_object *) i->callback);
}
struct lsh_class window_subscriber_class =
{
  STATIC_HEADER,
  &(resource_class),
  "window_subscriber",
  sizeof(struct window_subscriber),
  do_window_subscriber_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:unix_interact:interact
*/
#ifndef GABA_DEFINE
struct unix_interact
{
  struct interact super;
  int tty_fd;
  const char * askpass;
  struct resource *winch_handler;
  unsigned nsubscribers;
  struct window_subscriber *subscribers;
};
extern struct lsh_class unix_interact_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_unix_interact_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct unix_interact *i = (struct unix_interact *) o;
  mark((struct lsh_object *) i->winch_handler);
  mark((struct lsh_object *) i->subscribers);
}
struct lsh_class unix_interact_class =
{
  STATIC_HEADER,
  &(interact_class),
  "unix_interact",
  sizeof(struct unix_interact),
  do_unix_interact_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:unix_termios:terminal_attributes
*/
#ifndef GABA_DEFINE
struct unix_termios
{
  struct terminal_attributes super;
  struct termios ios;
};
extern struct lsh_class unix_termios_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class unix_termios_class =
{
  STATIC_HEADER,
  &(terminal_attributes_class),
  "unix_termios",
  sizeof(struct unix_termios),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:winch_handler:lsh_callback
*/
#ifndef GABA_DEFINE
struct winch_handler
{
  struct lsh_callback super;
  struct unix_interact *interact;
};
extern struct lsh_class winch_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_winch_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct winch_handler *i = (struct winch_handler *) o;
  mark((struct lsh_object *) i->interact);
}
struct lsh_class winch_handler_class =
{
  STATIC_HEADER,
  &(lsh_callback_class),
  "winch_handler",
  sizeof(struct winch_handler),
  do_winch_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

