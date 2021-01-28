#include "testutils.h"

int
test_main(void)
{
  test_cipher("CAST-128 CBC", &crypto_cast128_cbc_algorithm,
              H("0123456789ABCDEF FEDCBA9876543210"),
              H("0000000000000000 1111111111111111"),
	      H("9a1bf354bca596ad f7b2a2ce5f09a8b2"),
	      H("0011223344556677"));
  SUCCESS();
}
