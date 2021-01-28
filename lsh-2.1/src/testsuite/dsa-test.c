#include "testutils.h"

int
test_main(void)
{
  test_sign("DSA signing",
	    S("{KDM6ZHNhKDE6cDEyOToAg9mnws4qkXn0PNs7/+feDw7vJt1d+uRNUxvA3kVj"
	      "TSwHy5KbDb4Q2lgAcOar+7hBXES/9XC4rXed9lOq2X3HveuBXX6IED5hYG7T"
	      "2KKV+/00DS1J4iCDPrrOVRHiLE8Cl+01HplI+oSOnI+tt7R7zEfe9CVbXh1e"
	      "ECFbO1WguF8pKDE6cTIxOgCCZuDer0YCC6SNQQylgPOpeGKbXSkoMTpnMTI4"
	      "OjDTS7nzdr7JRxVK/kB2vH01nJ0y9Ucd276NapQcR/qdxPMlcxUdu0qlnrmJ"
	      "t0rDa7YxCl6LWAUBZV2R85PaoZOuEwMEm4f+uwk9wEBLU7TF2iRjMA+cWxVt"
	      "eIxKzo7Lud0AwY2ZU38lWsAl0HTYlKYHy+MCOhJ271VpFqM/feVDKSgxOnkx"
	      "Mjg6ZEAgSLJ/OfQEpUaoSQnJwOni3RU6hJlGEGKJJZjTCvJ64878K3APttB3"
	      "OQqDvcrXihKZSHyWI7tirwyFo9+e8e4sDWZljh/TKDtUB/bNMO5+YVT61Bpq"
	      "iw9chsWszBEnv3yaXWutywEhgMtipVxeF9bTUozb4ALM7hMcG4aGf3opKDE6"
	      "eDIwOlbG76+HjQbu8h3AcPq3HabsHjCmKSk=}"),
	    S("Needs some randomness."),
	    NULL);
  SUCCESS();
}
