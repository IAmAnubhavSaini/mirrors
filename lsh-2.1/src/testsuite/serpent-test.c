#include "testutils.h"

/* Note: In Nettle up to version 2.1, the serpent implementation did
   some broken byte reversal on input and output. This testcase will
   fail with old versions of nettle. */
int
test_main(void)
{
  test_cipher("Serpent-256 CBC", &crypto_serpent256_cbc_algorithm,
              H("0123456789ABCDEF FEDCBA9876543210"
		"0011223344556677 8899AABBCCDDEEFF"),
              H("0000000000000000 0000000000000000"
		"1111111111111111 1111111111111111"),
	      H("c170ed586cfda8fe 084f01ef04475883"
		"b9eb25819813023f 2938e97bdf4597c8"),
	      H("0011223344556677 8899AABBCCDDEEFF"));
  SUCCESS();
}
