#include "testutils.h"

int
test_main(void)
{
  test_cipher("Twofish-256 CBC", &crypto_twofish256_cbc_algorithm,
              H("0123456789ABCDEF FEDCBA9876543210"
		"0011223344556677 8899AABBCCDDEEFF"),
              H("0000000000000000 0000000000000000"
		"1111111111111111 1111111111111111"),
	      H("7c9cde6d86b1d9f2 9fceb6830c451281"
                "329f72e3eb36d505 6e8e08c191644dfa"),
	      H("0011223344556677 8899AABBCCDDEEFF"));
    
  SUCCESS();
}
