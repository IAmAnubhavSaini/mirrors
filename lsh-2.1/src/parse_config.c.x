/*
CLASS:config_setting:
*/
#ifndef GABA_DEFINE
struct config_setting
{
  struct lsh_object super;
  struct config_setting *next;
  enum config_type type;
  struct lsh_string *value;
};
extern struct lsh_class config_setting_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_config_setting_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct config_setting *i = (struct config_setting *) o;
  mark((struct lsh_object *) i->next);
}
static void
do_config_setting_free(struct lsh_object *o)
{
  struct config_setting *i = (struct config_setting *) o;
  lsh_string_free(i->value);
}
struct lsh_class config_setting_class =
{
  STATIC_HEADER,
  NULL,
  "config_setting",
  sizeof(struct config_setting),
  do_config_setting_mark,
  do_config_setting_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:config_host:
*/
#ifndef GABA_DEFINE
struct config_host
{
  struct lsh_object super;
  struct config_host *next;
  struct lsh_string *name;
  struct config_setting *settings;
};
extern struct lsh_class config_host_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_config_host_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct config_host *i = (struct config_host *) o;
  mark((struct lsh_object *) i->next);
  mark((struct lsh_object *) i->settings);
}
static void
do_config_host_free(struct lsh_object *o)
{
  struct config_host *i = (struct config_host *) o;
  lsh_string_free(i->name);
}
struct lsh_class config_host_class =
{
  STATIC_HEADER,
  NULL,
  "config_host",
  sizeof(struct config_host),
  do_config_host_mark,
  do_config_host_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:config_group:
*/
#ifndef GABA_DEFINE
struct config_group
{
  struct lsh_object super;
  struct config_group *next;
  struct lsh_string *name;
  struct config_setting *settings;
  struct config_host *hosts;
};
extern struct lsh_class config_group_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_config_group_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct config_group *i = (struct config_group *) o;
  mark((struct lsh_object *) i->next);
  mark((struct lsh_object *) i->settings);
  mark((struct lsh_object *) i->hosts);
}
static void
do_config_group_free(struct lsh_object *o)
{
  struct config_group *i = (struct config_group *) o;
  lsh_string_free(i->name);
}
struct lsh_class config_group_class =
{
  STATIC_HEADER,
  NULL,
  "config_group",
  sizeof(struct config_group),
  do_config_group_mark,
  do_config_group_free,
};
#endif /* !GABA_DECLARE */

