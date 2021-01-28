#include "testutils.h"

int
test_main(void)
{
  test_sign("RSA signing",
	    S("{KDE0OnJzYS1wa2NzMS1zaGExKDE6bjI1MzoBFRr6hWOxSyj4UOuRSv2TgnQT"
	      "RIbT+XhBCCuC0delL8hCt9SKN8QYGn9eSfJaMknWXZdbiATciNWXtC6BInC5"
	      "PiIyCwZP21u4x/UYNq7EpYRWfOr2GeFtgqAgjLCPiNcDmaje5RI6HltB1t69"
	      "gyRlgmTEGedjfCVUksR9HCa/YHO4Qj6nfGL43suz0qzbw90fo52pJQAFVsob"
	      "8uNXcIkKEIUF6iaCcFxz5chlmUDDuWZsu0CN2hwTVcNEODo2h4K9UZSJ1q5r"
	      "flh3Yd2hKeQDY4g1em/4faS43lvhkV+PqXNyaV5/9Oo5PfPhuVbppOvhV3fU"
	      "6GsPNH2rmaSnKSgxOmU0Og8eiqcpKDE6ZDI1MzoBALlbGgNWW/Iobv5Zm17I"
	      "lj8Vwg3DgYucIlNkKs4z+G5I1J5wSLlsSJp5mDixgy2meph8Zo7kRM8DXNbQ"
	      "FxPrx184sRHI+zQlcd61ldDSiy+U55cw99k+AW9xPGiSlIwfGFJb8+ihimFr"
	      "LD4bpJTjZmWFMza4W4EqovJZar2Lk8j1ahEClrVZxmClMwGViQf6sezat9eD"
	      "ccA3D1J7lbCqhhZ4/zO7M97VifRfcvzJCRsQUgf3fdsY6BFp0/jkx+7y0ZgC"
	      "IfBx7JJPPAe90wNIG7s+XXE4A0M3txusqFE3gpIZ+H84xD/as/h/CAkDnZDK"
	      "xTjaeL9KEMXaJDlXKSgxOnAxMjc6AMDrJe3pnACcCkZDTxmNifj/107wgUsy"
	      "qCX097sHwkRKRn4OGJYg1V8odSIBXiGVO9HvDqEZKwZQ7ck83IHavaJIEnTy"
	      "7vrSH3wS3x60PmQHINlqd3CnJISIKsvBnULrU8P892gxqTpqCEvpoo1ZnQzZ"
	      "qYNiAf5kj47TqJN2lykoMTpxMTI3OgFvtvQgJp9z2OJ+jJRPY3xPxK1wkymR"
	      "YSBAEjSTxfs7CiUjQ7cdWXlZoF/P9NGRVQErNkknFpVEC2Mh1qpIw4wN1ES/"
	      "Z54XTgshkibn2+wtDpD0Vj5r5S3UIta6iCn39SJbAUQSuMuuLLj5jwCfq8d/"
	      "V7BaU7aO58244vcFlHEpKDE6YTEyNjoiG57+FA/nD4jGoPHo5LPNSxecFmC8"
	      "YlC9r0ATzEB3xFW6g7qY8QYw2h/zmarUqO5KQVDxHwk+kJ24BIqMxDyqFdhj"
	      "r7c3bg7jSBv3MqSGfNWu/awoQ5rffKPDKN8TzWjrUJHYKQBjODfzmZuuWAsH"
	      "eEb4u7E4K8f3NLs14nMpKDE6YjEyNjouubKPLYPvxC3dj4WlXwRhfRgD2F/D"
	      "k2T00eEyJst/LLHFvNQ8H5/iB1FmUGWVcZAheYNFp2RzNU2+MoOgskKFQJ4q"
	      "VqX/A9mH9BULiEXMG1MC7hFT0O/w2bFBz6dFtpb50nPfO09J/G+DON1TnoOj"
	      "IxJ0RLy5Uq45jx9vPGcpKDE6YzEyNzoAs20M6PMchs+bdN/hiVdzD0PG5eOF"
	      "BCjR1tUbzRvhcA9zRW1ze0ozDyF1ZuqIqnoniHMz5Tm7P9swIioNNgrWys/S"
	      "k1nyPdDw6zkEj8mtW2RUyXb8FK+rgtE9UjTwTj2gZEsj8TjPE5WuaagBKe2K"
	      "tQ/OuMsYG7bMADddMRv4KSk=}"),
	    S("The magic words are squeamish ossifrage"),
	    NULL);
  SUCCESS();
}
