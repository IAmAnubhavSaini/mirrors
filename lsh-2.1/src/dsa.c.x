/*
CLASS:dsa_algorithm:signature_algorithm
*/
#ifndef GABA_DEFINE
struct dsa_algorithm
{
  struct signature_algorithm super;
  struct randomness *random;
};
extern struct lsh_class dsa_algorithm_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_dsa_algorithm_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct dsa_algorithm *i = (struct dsa_algorithm *) o;
  mark((struct lsh_object *) i->random);
}
struct lsh_class dsa_algorithm_class =
{
  STATIC_HEADER,
  &(signature_algorithm_class),
  "dsa_algorithm",
  sizeof(struct dsa_algorithm),
  do_dsa_algorithm_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:dsa_verifier:verifier
*/
#ifndef GABA_DEFINE
struct dsa_verifier
{
  struct verifier super;
  struct dsa_public_key key;
};
extern struct lsh_class dsa_verifier_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_dsa_verifier_free(struct lsh_object *o)
{
  struct dsa_verifier *i = (struct dsa_verifier *) o;
  dsa_public_key_clear(&(i->key));
}
struct lsh_class dsa_verifier_class =
{
  STATIC_HEADER,
  &(verifier_class),
  "dsa_verifier",
  sizeof(struct dsa_verifier),
  NULL,
  do_dsa_verifier_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:dsa_signer:signer
*/
#ifndef GABA_DEFINE
struct dsa_signer
{
  struct signer super;
  struct dsa_verifier *verifier;
  struct randomness *random;
  struct dsa_private_key key;
};
extern struct lsh_class dsa_signer_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_dsa_signer_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct dsa_signer *i = (struct dsa_signer *) o;
  mark((struct lsh_object *) i->verifier);
  mark((struct lsh_object *) i->random);
}
static void
do_dsa_signer_free(struct lsh_object *o)
{
  struct dsa_signer *i = (struct dsa_signer *) o;
  dsa_private_key_clear(&(i->key));
}
struct lsh_class dsa_signer_class =
{
  STATIC_HEADER,
  &(signer_class),
  "dsa_signer",
  sizeof(struct dsa_signer),
  do_dsa_signer_mark,
  do_dsa_signer_free,
};
#endif /* !GABA_DECLARE */

