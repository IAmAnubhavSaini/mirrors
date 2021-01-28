/*
CLASS:report_exception_handler:exception_handler
*/
#ifndef GABA_DEFINE
struct report_exception_handler
{
  struct exception_handler super;
  const struct report_exception_info *info;
};
extern struct lsh_class report_exception_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_report_exception_handler_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct report_exception_handler *i = (struct report_exception_handler *) o;
  mark((struct lsh_object *) i->info);
}
struct lsh_class report_exception_handler_class =
{
  STATIC_HEADER,
  &(exception_handler_class),
  "report_exception_handler",
  sizeof(struct report_exception_handler),
  do_report_exception_handler_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

