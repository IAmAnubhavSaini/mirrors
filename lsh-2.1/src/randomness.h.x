/*
CLASS:randomness:
*/
#ifndef GABA_DEFINE
struct randomness
{
  struct lsh_object super;
  enum randomness_quality quality;
  void (*(random))(struct randomness *self,uint32_t length,uint8_t *dst);
  void (*(add))(struct randomness *self,enum random_source_type,uint32_t length,const uint8_t *data);
};
extern struct lsh_class randomness_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class randomness_class =
{
  STATIC_HEADER,
  NULL,
  "randomness",
  sizeof(struct randomness),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

