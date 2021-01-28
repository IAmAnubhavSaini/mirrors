#include "testutils.h"

int
test_main(void)
{
  struct mac_algorithm *hmac
    = make_hmac_algorithm(&crypto_md5_algorithm);
  
  /* Test vectors from RFC 1321 */
  test_hash("MD5-1", &crypto_md5_algorithm,
	    S(""),
	    H("D41D8CD98F00B204 E9800998ECF8427E"));
  
  test_hash("MD5-2", &crypto_md5_algorithm,
	    S("a"),
	    H("0CC175B9C0F1B6A8 31C399E269772661"));
  
  test_hash("MD5-3", &crypto_md5_algorithm,
	    S("abc"),
	    H("900150983cd24fb0 D6963F7D28E17F72"));

  test_hash("MD5-4", &crypto_md5_algorithm,
	    S("message digest"),
	    H("F96B697D7CB7938D 525A2F31AAF161D0"));

  test_hash("MD5-5", &crypto_md5_algorithm,
	    S("abcdefghijklmnopqrstuvwxyz"),
	    H("C3FCD3D76192E400 7DFB496CCA67E13B"));

  test_hash("MD5-6", &crypto_md5_algorithm,
	    S("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
	    H("D174AB98D277D9F5 A5611C2C9F419D9F"));

  test_hash("MD5-7", &crypto_md5_algorithm,
	    S("1234567890123456789012345678901234567890"
	      "1234567890123456789012345678901234567890"),
	    H("57EDF4A22BE3C955 AC49DA2E2107B67A"));

  /* Test vectors for md5 from RFC-2202 */
  test_mac("HMAC-MD5-1", hmac,
	   H("0b0b0b0b0b0b0b0b 0b0b0b0b0b0b0b0b"),
	   S("Hi There"),
	   H("9294727A3638BB1C 13F48EF8158BFC9D"));

  test_mac("HMAC-MD5-2", hmac,
	   S("Jefe"),
	   S("what do ya want for nothing?"),
	   H("750C783E6AB0B503 EAA86E310A5DB738"));

  test_mac("HMAC-MD5-3", hmac,
	   H("AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"),
	   H("DDDDDDDDDDDDDDDD DDDDDDDDDDDDDDDD"
	     "DDDDDDDDDDDDDDDD DDDDDDDDDDDDDDDD"
	     "DDDDDDDDDDDDDDDD DDDDDDDDDDDDDDDD"
	     "DDDD"),
	   H("56BE34521D144C88 DBB8C733F0E8B3F6"));

  test_mac("HMAC-MD5-4", hmac,
	   H("0102030405060708 090A0B0C0D0E0F10 1112131415161718 19"),
	   H("CDCDCDCDCDCDCDCD CDCDCDCDCDCDCDCD"
	     "CDCDCDCDCDCDCDCD CDCDCDCDCDCDCDCD"
	     "CDCDCDCDCDCDCDCD CDCDCDCDCDCDCDCD"
	     "CDCD"),
	   H("697EAF0ACA3A3AEA 3A75164746FFAA79"));

  test_mac("HMAC-MD5-5", hmac,
	   H("0C0C0C0C0C0C0C0C 0C0C0C0C0C0C0C0C"),
	   S("Test With Truncation"),
	   H("56461EF2342EDC00 F9BAB995690EFD4C"));

  test_mac("HMAC-MD5-6", hmac,
	   H("AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"),
	   S("Test Using Larger Than Block-Size Key - Hash Key First"),
	   H("6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd"));

  test_mac("HMAC-MD5-7", hmac,
	   H("AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"),
	   S("Test Using Larger Than Block-Size Key an"
	     "d Larger Than One Block-Size Data"),
	   H("6f630fad67cda0ee1fb1f562db3aa53e"));

  SUCCESS();
}
