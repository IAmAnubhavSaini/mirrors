#include "testutils.h"

void
read_acl(struct spki_acl_db *db,
	 unsigned length, const uint8_t *data)
{
  struct spki_iterator i;

  ASSERT(spki_iterator_first(&i, length, data) == SPKI_TYPE_ACL);
  ASSERT(spki_acl_process(db, &i));
}

struct spki_tag *
make_tag(unsigned length, const uint8_t *expr)
{
  struct spki_tag *tag = spki_tag_from_sexp(NULL, nettle_realloc,
					    length, expr);
  ASSERT(tag);
  
  return tag;
}

void
release_tag(struct spki_tag *tag)
{
  spki_tag_release(NULL, nettle_realloc, tag);
}

int
main(int argc UNUSED, char **argv UNUSED)
{
  test_main();

  return EXIT_SUCCESS;
}
