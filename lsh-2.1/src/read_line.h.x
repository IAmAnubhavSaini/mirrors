/*
CLASS:line_handler:
*/
#ifndef GABA_DEFINE
struct line_handler
{
  struct lsh_object super;
  void (*(handler))(struct line_handler **self,struct read_handler **r,uint32_t length,const uint8_t *line,struct exception_handler *e);
};
extern struct lsh_class line_handler_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class line_handler_class =
{
  STATIC_HEADER,
  NULL,
  "line_handler",
  sizeof(struct line_handler),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

