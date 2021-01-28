#include "testutils.h"

void
test_main(void)
{
  struct spki_acl_db db;
  struct spki_principal *s;
  
  spki_acl_init(&db);
  
  s = spki_principal_by_key(&db, 5, "3:foo");
  ASSERT(s);

  ASSERT(spki_principal_by_key(&db, 5, "3:foo") == s);
  ASSERT(spki_principal_by_key(&db, 5, "3:bar") != s);
}
