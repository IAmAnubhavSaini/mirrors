#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "certificate.h"
#include "parse.h"
#include "tag.h"

#include <nettle/sexp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
read_acl(struct spki_acl_db *db,
	 unsigned length, const uint8_t *data);

struct spki_tag *
make_tag(unsigned length, const uint8_t *expr);

void
release_tag(struct spki_tag *tag);

#define FAIL(msg) do { fprintf(stderr, "%s\n", msg); abort(); } while(0)

#define ASSERT(x) do { if (!(x)) FAIL("ASSERT failure: " #x); } while(0)

#define LLENGTH(x) (sizeof(x) - 1)
#define LDATA(x) (sizeof(x) - 1), x

void
test_main(void);
