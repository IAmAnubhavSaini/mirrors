/*
CLASS:crypto_instance:
*/
#ifndef GABA_DEFINE
struct crypto_instance
{
  struct lsh_object super;
  uint32_t block_size;
  void (*(crypt))(struct crypto_instance *self,uint32_t length,struct lsh_string *dst,uint32_t di,const struct lsh_string *src,uint32_t si);
};
extern struct lsh_class crypto_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class crypto_instance_class =
{
  STATIC_HEADER,
  NULL,
  "crypto_instance",
  sizeof(struct crypto_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:crypto_algorithm:
*/
#ifndef GABA_DEFINE
struct crypto_algorithm
{
  struct lsh_object super;
  uint32_t block_size;
  uint32_t key_size;
  uint32_t iv_size;
  struct crypto_instance *(*(make_crypt))(struct crypto_algorithm *self,int mode,const uint8_t *key,const uint8_t *iv);
};
extern struct lsh_class crypto_algorithm_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class crypto_algorithm_class =
{
  STATIC_HEADER,
  NULL,
  "crypto_algorithm",
  sizeof(struct crypto_algorithm),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:hash_instance:
*/
#ifndef GABA_DEFINE
struct hash_instance
{
  struct lsh_object super;
  const struct nettle_hash * type;
  char ((ctx)[1]);
};
extern struct lsh_class hash_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class hash_instance_class =
{
  STATIC_HEADER,
  NULL,
  "hash_instance",
  sizeof(struct hash_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:hash_algorithm:
*/
#ifndef GABA_DEFINE
struct hash_algorithm
{
  struct lsh_object super;
  const struct nettle_hash * type;
};
extern struct lsh_class hash_algorithm_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class hash_algorithm_class =
{
  STATIC_HEADER,
  NULL,
  "hash_algorithm",
  sizeof(struct hash_algorithm),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:mac_instance:
*/
#ifndef GABA_DEFINE
struct mac_instance
{
  struct lsh_object super;
  uint32_t mac_size;
  void (*(update))(struct mac_instance *self,uint32_t length,const uint8_t *data);
  struct lsh_string * (*(digest))(struct mac_instance *self,struct lsh_string *res,uint32_t pos);
};
extern struct lsh_class mac_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class mac_instance_class =
{
  STATIC_HEADER,
  NULL,
  "mac_instance",
  sizeof(struct mac_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:mac_algorithm:
*/
#ifndef GABA_DEFINE
struct mac_algorithm
{
  struct lsh_object super;
  uint32_t mac_size;
  uint32_t key_size;
  struct mac_instance *(*(make_mac))(struct mac_algorithm *self,uint32_t length,const uint8_t *key);
};
extern struct lsh_class mac_algorithm_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class mac_algorithm_class =
{
  STATIC_HEADER,
  NULL,
  "mac_algorithm",
  sizeof(struct mac_algorithm),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:verifier:
*/
#ifndef GABA_DEFINE
struct verifier
{
  struct lsh_object super;
  int (*(verify))(struct verifier *self,int algorithm,uint32_t length,const uint8_t *data,uint32_t signature_length,const uint8_t *signature_data);
  struct lsh_string *(*(public_key))(struct verifier *self);
  struct lsh_string *(*(public_spki_key))(struct verifier *self,int transport);
};
extern struct lsh_class verifier_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class verifier_class =
{
  STATIC_HEADER,
  NULL,
  "verifier",
  sizeof(struct verifier),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:signer:
*/
#ifndef GABA_DEFINE
struct signer
{
  struct lsh_object super;
  struct lsh_string *(*(sign))(struct signer *self,int algorithm,uint32_t length,const uint8_t *data);
  struct verifier *(*(get_verifier))(struct signer *self);
};
extern struct lsh_class signer_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class signer_class =
{
  STATIC_HEADER,
  NULL,
  "signer",
  sizeof(struct signer),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:signature_algorithm:
*/
#ifndef GABA_DEFINE
struct signature_algorithm
{
  struct lsh_object super;
  struct signer *(*(make_signer))(struct signature_algorithm *self,struct sexp_iterator *i);
  struct verifier *(*(make_verifier))(struct signature_algorithm *self,struct sexp_iterator *i);
};
extern struct lsh_class signature_algorithm_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class signature_algorithm_class =
{
  STATIC_HEADER,
  NULL,
  "signature_algorithm",
  sizeof(struct signature_algorithm),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

