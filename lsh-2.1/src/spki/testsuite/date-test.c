#include "testutils.h"

void
test_main(void)
{
  struct spki_date d;

  spki_date_from_time_t(&d, 1037568583);

  ASSERT(!memcmp(d.date, "2002-11-17_21:29:43", SPKI_DATE_SIZE));
  ASSERT(spki_date_cmp_time_t(&d, 1037568583) == 0);
  ASSERT(spki_date_cmp_time_t(&d, 1037568547) > 0);
  ASSERT(spki_date_cmp_time_t(&d, 1037568587) < 0);
}
