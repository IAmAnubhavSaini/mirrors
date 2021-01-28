#include "testutils.h"

int
test_main(void)
{
  test_cipher("DES3 CBC", &crypto_des3_cbc_algorithm,
	      H("0123456789ABCDEF 1313131313131313 3232323232323232"),
	      H("0011223344556677 8899AABBCCDDEEFF"),
	      H("C4DB96109B6186B2 5F9AA359160909D9"),
	      H("0001020304050607"));
  SUCCESS();
}
