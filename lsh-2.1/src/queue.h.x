#ifndef GABA_DEFINE
struct object_queue
{
  struct lsh_queue q;
};
extern void object_queue_mark(struct object_queue *i,
 void (*mark)(struct lsh_object *o));
extern void object_queue_free(struct object_queue *i);
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
void object_queue_mark(struct object_queue *i,
 void (*mark)(struct lsh_object *o))
{
  (void) mark; (void) i;
  do_object_queue_mark(&(i->q),
    mark);
}
void object_queue_free(struct object_queue *i)
{
  (void) i;
  do_object_queue_free(&(i->q));
}
#endif /* !GABA_DECLARE */

#ifndef GABA_DEFINE
struct string_queue
{
  struct lsh_queue q;
};
extern void string_queue_mark(struct string_queue *i,
 void (*mark)(struct lsh_object *o));
extern void string_queue_free(struct string_queue *i);
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
void string_queue_mark(struct string_queue *i,
 void (*mark)(struct lsh_object *o))
{
  (void) mark; (void) i;
}
void string_queue_free(struct string_queue *i)
{
  (void) i;
  do_string_queue_free(&(i->q));
}
#endif /* !GABA_DECLARE */

#ifndef GABA_DEFINE
struct addr_queue
{
  struct lsh_queue q;
};
extern void addr_queue_mark(struct addr_queue *i,
 void (*mark)(struct lsh_object *o));
extern void addr_queue_free(struct addr_queue *i);
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
void addr_queue_mark(struct addr_queue *i,
 void (*mark)(struct lsh_object *o))
{
  (void) mark; (void) i;
}
void addr_queue_free(struct addr_queue *i)
{
  (void) i;
  do_addr_queue_free(&(i->q));
}
#endif /* !GABA_DECLARE */

