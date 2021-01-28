/*
CLASS:exception:
*/
#ifndef GABA_DEFINE
struct exception
{
  struct lsh_object super;
  uint32_t type;
  const char * msg;
};
extern struct lsh_class exception_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class exception_class =
{
  STATIC_HEADER,
  NULL,
  "exception",
  sizeof(struct exception),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:exception_handler:
*/
#ifndef GABA_DEFINE
struct exception_handler
{
  struct lsh_object super;
  void (*(raise))(struct exception_handler *self,const struct exception *);
  struct exception_handler *parent;
  const char * context;
};
extern struct lsh_class exception_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_exception_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct exception_handler *i = (struct exception_handler *) o;
  mark((struct lsh_object *) i->parent);
}
struct lsh_class exception_handler_class =
{
  STATIC_HEADER,
  NULL,
  "exception_handler",
  sizeof(struct exception_handler),
  do_exception_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:report_exception_info:
*/
#ifndef GABA_DEFINE
struct report_exception_info
{
  struct lsh_object super;
  uint32_t mask;
  uint32_t value;
  const char * prefix;
};
extern struct lsh_class report_exception_info_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class report_exception_info_class =
{
  STATIC_HEADER,
  NULL,
  "report_exception_info",
  sizeof(struct report_exception_info),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:protocol_exception:exception
*/
#ifndef GABA_DEFINE
struct protocol_exception
{
  struct exception super;
  uint32_t reason;
};
extern struct lsh_class protocol_exception_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class protocol_exception_class =
{
  STATIC_HEADER,
  &(exception_class),
  "protocol_exception",
  sizeof(struct protocol_exception),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

