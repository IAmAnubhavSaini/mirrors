#include "testutils.h"

int
test_main(void)
{
  struct mac_algorithm *hmac
    = make_hmac_algorithm(&crypto_sha1_algorithm);
  
  test_hash("SHA1", &crypto_sha1_algorithm,
	    S("abc"),
	    H("A9993E364706816A BA3E25717850C26C 9CD0D89D"));

  /* Test vectors for sha1 from RFC-2202, and some more */

  test_mac("HMAC-SHA1-0", hmac,
	   H("0b0b0b0b0b0b0b0b 0b0b0b0b0b0b0b0b"),
	   S("Hi There"),
	   H("675B0B3A1B4DDF4E 124872DA6C2F632B FED957E9"));

  test_mac("HMAC-SHA1-1", hmac,
	   H("0b0b0b0b0b0b0b0b 0b0b0b0b0b0b0b0b 0b0b0b0b"),
	   S("Hi There"),
	   H("B617318655057264 E28BC0B6FB378C8E F146BE00"));

  test_mac("HMAC-SHA1-2", hmac,
	   S("Jefe"),
	   S("what do ya want for nothing?"),
	   H("EFFCDF6AE5EB2FA2 D27416D5F184DF9C 259A7C79"));

  test_mac("HMAC-SHA1-3a", hmac,
	   H("AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"),
	   H("DDDDDDDDDDDDDDDD DDDDDDDDDDDDDDDD"
	     "DDDDDDDDDDDDDDDD DDDDDDDDDDDDDDDD"
	     "DDDDDDDDDDDDDDDD DDDDDDDDDDDDDDDD"
	     "DDDD"),
	   H("D730594D167E35D5 956FD8003D0DB3D3 F46DC7BB"));

  test_mac("HMAC-SHA1-3", hmac,
	   H("AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA AAAAAAAA"),
	   H("DDDDDDDDDDDDDDDD DDDDDDDDDDDDDDDD"
	     "DDDDDDDDDDDDDDDD DDDDDDDDDDDDDDDD"
	     "DDDDDDDDDDDDDDDD DDDDDDDDDDDDDDDD"
	     "DDDD"),
	   H("125D7342B9AC11CD 91A39AF48AA17B4F 63F175D3"));

  test_mac("HMAC-SHA1-4", hmac,
	   H("0102030405060708 090A0B0C0D0E0F10 1112131415161718 19"),
	   H("CDCDCDCDCDCDCDCD CDCDCDCDCDCDCDCD"
	     "CDCDCDCDCDCDCDCD CDCDCDCDCDCDCDCD"
	     "CDCDCDCDCDCDCDCD CDCDCDCDCDCDCDCD"
	     "CDCD"),
	   H("4C9007F4026250C6 BC8414F9BF50C86C 2D7235DA"));

  test_mac("HMAC-SHA1-5", hmac,
	   H("0C0C0C0C0C0C0C0C 0C0C0C0C0C0C0C0C 0C0C0C0C"),
	   S("Test With Truncation"),
	   H("4C1A03424B55E07F E7F27BE1D58BB932 4A9A5A04"));

  test_mac("HMAC-SHA1-6", hmac,
	   H("AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"),
	   S("Test Using Larger Than Block-Size Key - Hash Key First"),
	   H("AA4AE5E15272D00E 95705637CE8A3B55 ED402112"));

  test_mac("HMAC-SHA1-7", hmac,
	   H("AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"
	     "AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA"),
	   S("Test Using Larger Than Block-Size Key an"
	     "d Larger Than One Block-Size Data"),
	   H("E8E99D0F45237D78 6D6BBAA7965C7808 BBFF1A91"));

  SUCCESS();
}