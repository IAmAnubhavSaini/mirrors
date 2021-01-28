#include "testutils.h"

int
test_main(void)
{
  test_cipher("Arcfour", &crypto_arcfour_algorithm,
	      H("01234567 89ABCDEF 00000000 00000000"),
	      H("01234567 89ABCDEF"),
	      H("69723659 1B5242B1"),
	      NULL);
  SUCCESS();
}

