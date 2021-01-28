#include "testutils.h"

int
test_main(void)
{
  /* Examples taken from RFC-2693 */
  test_spki_grant("1",
		  S("(3:tag(3:ftp13:ftp.clark.net3:cme(1:*3:set4:read5:write)))"),
		  S("(3:tag(3:ftp13:ftp.clark.net3:cme4:read))"));

  test_spki_deny("2",
		 S("(3:tag(3:ftp13:ftp.clark.net3:cme(1:*3:set4:read5:write)))"),
		 S("(3:tag(3:ftp13:ftp.clark.net3:cme6:delete))"));

  test_spki_deny("3",
		 S("(3:tag(3:ftp13:ftp.clark.net3:cme(1:*3:set4:read5:write)))"),
		 S("(3:tag(3:ftp13:ftp.clark.net3:cme))"));

  SUCCESS();
}

