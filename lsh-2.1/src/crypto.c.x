/*
CLASS:arcfour_instance:crypto_instance
*/
#ifndef GABA_DEFINE
struct arcfour_instance
{
  struct crypto_instance super;
  struct arcfour_ctx ctx;
};
extern struct lsh_class arcfour_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class arcfour_instance_class =
{
  STATIC_HEADER,
  &(crypto_instance_class),
  "arcfour_instance",
  sizeof(struct arcfour_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:aes_cbc_instance:crypto_instance
*/
#ifndef GABA_DEFINE
struct aes_cbc_instance
{
  struct crypto_instance super;
  struct CBC_CTX(struct aes_ctx, AES_BLOCK_SIZE) ctx;
};
extern struct lsh_class aes_cbc_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class aes_cbc_instance_class =
{
  STATIC_HEADER,
  &(crypto_instance_class),
  "aes_cbc_instance",
  sizeof(struct aes_cbc_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:aes_ctr_instance:crypto_instance
*/
#ifndef GABA_DEFINE
struct aes_ctr_instance
{
  struct crypto_instance super;
  struct CTR_CTX(struct aes_ctx, AES_BLOCK_SIZE) ctx;
};
extern struct lsh_class aes_ctr_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class aes_ctr_instance_class =
{
  STATIC_HEADER,
  &(crypto_instance_class),
  "aes_ctr_instance",
  sizeof(struct aes_ctr_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:des3_instance:crypto_instance
*/
#ifndef GABA_DEFINE
struct des3_instance
{
  struct crypto_instance super;
  struct CBC_CTX(struct des3_ctx, DES3_BLOCK_SIZE) ctx;
};
extern struct lsh_class des3_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class des3_instance_class =
{
  STATIC_HEADER,
  &(crypto_instance_class),
  "des3_instance",
  sizeof(struct des3_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:cast128_instance:crypto_instance
*/
#ifndef GABA_DEFINE
struct cast128_instance
{
  struct crypto_instance super;
  struct CBC_CTX(struct cast128_ctx, CAST128_BLOCK_SIZE) ctx;
};
extern struct lsh_class cast128_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class cast128_instance_class =
{
  STATIC_HEADER,
  &(crypto_instance_class),
  "cast128_instance",
  sizeof(struct cast128_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:twofish_instance:crypto_instance
*/
#ifndef GABA_DEFINE
struct twofish_instance
{
  struct crypto_instance super;
  struct CBC_CTX(struct twofish_ctx, TWOFISH_BLOCK_SIZE) ctx;
};
extern struct lsh_class twofish_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class twofish_instance_class =
{
  STATIC_HEADER,
  &(crypto_instance_class),
  "twofish_instance",
  sizeof(struct twofish_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:blowfish_instance:crypto_instance
*/
#ifndef GABA_DEFINE
struct blowfish_instance
{
  struct crypto_instance super;
  struct CBC_CTX(struct blowfish_ctx, BLOWFISH_BLOCK_SIZE) ctx;
};
extern struct lsh_class blowfish_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class blowfish_instance_class =
{
  STATIC_HEADER,
  &(crypto_instance_class),
  "blowfish_instance",
  sizeof(struct blowfish_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:serpent_instance:crypto_instance
*/
#ifndef GABA_DEFINE
struct serpent_instance
{
  struct crypto_instance super;
  struct CBC_CTX(struct serpent_ctx, SERPENT_BLOCK_SIZE) ctx;
};
extern struct lsh_class serpent_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class serpent_instance_class =
{
  STATIC_HEADER,
  &(crypto_instance_class),
  "serpent_instance",
  sizeof(struct serpent_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:hmac_instance:mac_instance
*/
#ifndef GABA_DEFINE
struct hmac_instance
{
  struct mac_instance super;
  const struct nettle_hash * type;
  char ((ctx)[1]);
};
extern struct lsh_class hmac_instance_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class hmac_instance_class =
{
  STATIC_HEADER,
  &(mac_instance_class),
  "hmac_instance",
  sizeof(struct hmac_instance),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

/*
CLASS:hmac_algorithm:mac_algorithm
*/
#ifndef GABA_DEFINE
struct hmac_algorithm
{
  struct mac_algorithm super;
  const struct nettle_hash * type;
};
extern struct lsh_class hmac_algorithm_class;
#endif /* !GABA_DEFINE */

#ifndef GABA_DECLARE
struct lsh_class hmac_algorithm_class =
{
  STATIC_HEADER,
  &(mac_algorithm_class),
  "hmac_algorithm",
  sizeof(struct hmac_algorithm),
  NULL,
  NULL,
};
#endif /* !GABA_DECLARE */

