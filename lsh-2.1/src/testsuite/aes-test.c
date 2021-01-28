#include "testutils.h"

int
test_main(void)
{
  /* From draft NIST spec on AES modes. */

  test_cipher("aes-256 cbc 1", &crypto_aes256_cbc_algorithm, 
	      H("603deb1015ca71be2b73aef0857d7781"
		"1f352c073b6108d72d9810a30914dff4"),
	      H("6bc1bee22e409f96e93d7e117393172a"
		"ae2d8a571e03ac9c9eb76fac45af8e51"
		"30c81c46a35ce411e5fbc1191a0a52ef"
		"f69f2445df4f9b17ad2b417be66c3710"),
	      H("f58c4c04d6e5f1ba779eabfb5f7bfbd6"
		"9cfc4e967edb808d679f777bc6702c7d"
		"39f23369a9d9bacfa530e26304231461"
		"b2eb05e2c39be9fcda6c19078c6a9d1b"),
	      H("000102030405060708090a0b0c0d0e0f"));

  SUCCESS();
}
