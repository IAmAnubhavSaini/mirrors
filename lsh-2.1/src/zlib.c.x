/*
CLASS:zlib_instance:compress_instance
*/
#ifndef GABA_DEFINE
struct zlib_instance
{
  struct compress_instance super;
  uint32_t max;
  uint32_t rate;
  int (*(f))(z_stream *,int);
  z_stream z;
};
extern struct lsh_class zlib_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_zlib_instance_free(struct lsh_object *o)
{
  struct zlib_instance *i = (struct zlib_instance *) o;
  do_free_zstream(&(i->z));
}
struct lsh_class zlib_instance_class =
{
  STATIC_HEADER,
  &(compress_instance_class),
  "zlib_instance",
  sizeof(struct zlib_instance),
  NULL,
  do_zlib_instance_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:zlib_algorithm:compress_algorithm
*/
#ifndef GABA_DEFINE
struct zlib_algorithm
{
  struct compress_algorithm super;
  int level;
};
extern struct lsh_class zlib_algorithm_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class zlib_algorithm_class =
{
  STATIC_HEADER,
  &(compress_algorithm_class),
  "zlib_algorithm",
  sizeof(struct zlib_algorithm),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

