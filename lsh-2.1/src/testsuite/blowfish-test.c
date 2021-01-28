#include "testutils.h"

int
test_main(void)
{
  test_cipher("Blowfish-128 CBC", &crypto_blowfish_cbc_algorithm,
              H("0123456789ABCDEF FEDCBA9876543210"),
              H("0000000000000000 1111111111111111"),
	      H("f0fb0320a19f306d ff5880191e616c10"),
	      H("0011223344556677"));
  SUCCESS();
}
