/*
CLASS:keypair:
*/
#ifndef GABA_DEFINE
struct keypair
{
  struct lsh_object super;
  int type;
  struct lsh_string *public;
  struct signer *private;
};
extern struct lsh_class keypair_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_keypair_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct keypair *i = (struct keypair *) o;
  mark((struct lsh_object *) i->private);
}
static void
do_keypair_free(struct lsh_object *o)
{
  struct keypair *i = (struct keypair *) o;
  lsh_string_free(i->public);
}
struct lsh_class keypair_class =
{
  STATIC_HEADER,
  NULL,
  "keypair",
  sizeof(struct keypair),
  do_keypair_mark,
  do_keypair_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:zn_group:
*/
#ifndef GABA_DEFINE
struct zn_group
{
  struct lsh_object super;
  mpz_t modulo;
  mpz_t generator;
  mpz_t order;
};
extern struct lsh_class zn_group_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_zn_group_free(struct lsh_object *o)
{
  struct zn_group *i = (struct zn_group *) o;
  mpz_clear(i->modulo);
  mpz_clear(i->generator);
  mpz_clear(i->order);
}
struct lsh_class zn_group_class =
{
  STATIC_HEADER,
  NULL,
  "zn_group",
  sizeof(struct zn_group),
  NULL,
  do_zn_group_free,
};
#endif /* !GABA_DECLARE */

/*
CLASS:dh_method:
*/
#ifndef GABA_DEFINE
struct dh_method
{
  struct lsh_object super;
  const struct zn_group *G;
  const struct hash_algorithm *H;
  struct randomness *random;
};
extern struct lsh_class dh_method_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
static void
do_dh_method_mark(struct lsh_object *o,
  void (*mark)(struct lsh_object *o))
{
  struct dh_method *i = (struct dh_method *) o;
  mark((struct lsh_object *) i->G);
  mark((struct lsh_object *) i->H);
  mark((struct lsh_object *) i->random);
}
struct lsh_class dh_method_class =
{
  STATIC_HEADER,
  NULL,
  "dh_method",
  sizeof(struct dh_method),
  do_dh_method_mark,
  NULL,
};
#endif /* !GABA_DECLARE */

#ifndef GABA_DEFINE
struct dh_instance
{
  const struct dh_method *method;
  mpz_t e;
  mpz_t f;
  mpz_t secret;
  struct lsh_string *K;
  struct hash_instance *hash;
  struct lsh_string *exchange_hash;
};
extern void dh_instance_mark(struct dh_instance *i,
 void (*mark)(struct lsh_object *o));
extern void dh_instance_free(struct dh_instance *i);
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
void dh_instance_mark(struct dh_instance *i,
 void (*mark)(struct lsh_object *o))
{
  (void) mark; (void) i;
  mark((struct lsh_object *) i->method);
  mark((struct lsh_object *) i->hash);
}
void dh_instance_free(struct dh_instance *i)
{
  (void) i;
  mpz_clear(i->e);
  mpz_clear(i->f);
  mpz_clear(i->secret);
  lsh_string_free(i->K);
  lsh_string_free(i->exchange_hash);
}
#endif /* !GABA_DECLARE */

