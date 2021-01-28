/*
CLASS:rsa_verifier:verifier
*/
#ifndef GABA_DEFINE
struct rsa_verifier
{
  struct verifier super;
  struct rsa_public_key key;
};
extern struct lsh_class rsa_verifier_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_rsa_verifier_free(struct lsh_object *o)
{
  struct rsa_verifier *i = (struct rsa_verifier *) o;
  rsa_public_key_clear(&(i->key));
}
struct lsh_class rsa_verifier_class =
{
  STATIC_HEADER,
  &(verifier_class),
  "rsa_verifier",
  sizeof(struct rsa_verifier),
  NULL,
  do_rsa_verifier_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:rsa_signer:signer
*/
#ifndef GABA_DEFINE
struct rsa_signer
{
  struct signer super;
  struct rsa_verifier *verifier;
  struct rsa_private_key key;
};
extern struct lsh_class rsa_signer_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_rsa_signer_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct rsa_signer *i = (struct rsa_signer *) o;
  mark((struct lsh_object *) i->verifier);
}
static void
do_rsa_signer_free(struct lsh_object *o)
{
  struct rsa_signer *i = (struct rsa_signer *) o;
  rsa_private_key_clear(&(i->key));
}
struct lsh_class rsa_signer_class =
{
  STATIC_HEADER,
  &(signer_class),
  "rsa_signer",
  sizeof(struct rsa_signer),
  do_rsa_signer_mark,
  do_rsa_signer_free,
};
#endif /* !GABA_DECLARE */

