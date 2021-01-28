#include "testutils.h"

/* Handle testcases snarfed from CDSA. */

#include <assert.h>
#include <stdio.h>

static char *
read_file(const char *srcdir,
	  const char *prefix, unsigned i, const char *suffix,
	  unsigned *length)
{
  unsigned srcdir_length = srcdir ? strlen(srcdir) : 0;
  char *fname = alloca(srcdir_length + strlen(prefix) + strlen(suffix) + 100);
  FILE *f;
  unsigned done = 0;
  unsigned alloc = 0;
  char *buffer = NULL;
  
  if (srcdir)
    sprintf(fname, "%s/%s%d%s", srcdir, prefix, i, suffix);
  else
    sprintf(fname, "%s%d%s", prefix, i, suffix);

  f = fopen(fname, "rb");
  if (!f)
    return NULL;
  
  for (;;)
    {
      assert(alloc == done);
      
      alloc = alloc * 2 + 100;
      buffer = realloc(buffer, alloc);
      if (!buffer)
	{
	  fprintf(stderr, "Virtual memory exhausted.\n");
	  abort();
	}

      done += fread(buffer + done, 1, alloc - done, f);

      if (done < alloc)
	{
	  if (ferror(f))
	    {
	      fprintf(stderr, "Read error on file `%s'\n", fname);
	      exit(EXIT_FAILURE);
	    }
	  else if (feof(f))
	    {
	      *length = done;
	      return buffer;
	    }
	  abort();
	}
    }
}

static int
filter_by_tag(struct spki_acl_db *db,
	      void *ctx,
	      struct spki_5_tuple *acl)
{
  struct spki_tag *tag = ctx;
  struct spki_tag *intersection
    = spki_tag_intersect(db->realloc_ctx, db->realloc,
			 tag, acl->tag);

  if (intersection)
    {
      /* NOTE: Destructive change */
      spki_tag_release(db->realloc_ctx, db->realloc, acl->tag);
      acl->tag = intersection;
      return 1;
    }
  else
    return 0;
}

static int
filter_by_subject(struct spki_acl_db *db UNUSED,
		  void *ctx,
		  struct spki_5_tuple *acl)
{
  struct spki_principal *subject = ctx;

  return (subject == spki_principal_normalize(acl->subject));
}

static int
filter_by_date(struct spki_acl_db *db UNUSED,
	       void *ctx,
	       struct spki_5_tuple *acl)
{
  struct spki_5_tuple *date = ctx;

  /* NOTE: Destructive operations */
  if (SPKI_DATE_CMP(acl->not_before, date->not_before) < 0)
    acl->not_before = date->not_before;

  if (SPKI_DATE_CMP(acl->not_after, date->not_after) > 0)
    acl->not_after = date->not_after;
      
  return (SPKI_DATE_CMP(acl->not_before, acl->not_after) <= 0);
}

void
test_main(void)
{
  unsigned i;
  const char *srcdir = getenv("srcdir");
  
  for (i = 1; i <= 91; i++)
    {
      struct spki_acl_db db;      
      struct spki_5_tuple_list *result;

      if (i == 13 /* This test uses an acl with empty tag. Skip it */
	  || i == 18 /* This tests uses a validity filter with dates missing
		      * seconds. */
	  || i == 19
	  || i == 20 /* This tests uses a validity filter with microseconds. */
	  )
	continue;

      if (i == 27)
	/* Rest of the test cases use features that haven't been
	 * implemented yet. */
	break;
      {
	struct sexp_iterator sexp;
	struct spki_iterator iterator;
	
	unsigned length;
	uint8_t *data;

	fprintf(stderr, "i: %d\n", i);
	
	data = read_file(srcdir, "cdsa-cases/", i, ".in",
			 &length);
	
	ASSERT(sexp_iterator_first(&sexp, length, data));
	ASSERT(sexp_iterator_check_type(&sexp, "red-test"));

	ASSERT(spki_iterator_first_sexp(&iterator, &sexp));
	       
	/* A "red-test" contains an acl and a sequence */
	spki_acl_init(&db);
	ASSERT(spki_acl_process(&db, &iterator));

	if (iterator.type == SPKI_TYPE_SEQUENCE)
	  {
	    struct spki_5_tuple_list *sequence;
	    const struct spki_principal *subject;

	    ASSERT(spki_parse_sequence_no_signatures(&db, &iterator,
						     &sequence, &subject));

	    ASSERT(subject);
	    result = spki_5_tuple_reduce(&db, sequence);

	    spki_5_tuple_list_release(&db, sequence);
	  }
	else
	  {
	    /* Just use the ACL:s */
	    result = db.acls;
	    db.acls = NULL;
	  }

	if (iterator.type == SPKI_TYPE_TAG)
	  {
	    struct spki_tag *tag;
	    ASSERT(spki_parse_tag(&db, &iterator, &tag));
	    
	    result = spki_5_tuple_list_filter(&db, result,
					      tag, filter_by_tag);
	  }
	ASSERT(result);

	if (iterator.type == SPKI_TYPE_SUBJECT)
	  {
	    struct spki_principal *subject;
	    ASSERT(spki_parse_subject(&db, &iterator, &subject));

	    result = spki_5_tuple_list_filter(&db, result,
					      subject, filter_by_subject);
	  }
	ASSERT(result);

	if (iterator.type == SPKI_TYPE_VALID)
	  {
	    struct spki_5_tuple tuple;

	    if (iterator.sexp.type == SEXP_ATOM)
	      {
		/* We have only a single date */
		ASSERT(spki_parse_date(&iterator, &tuple.not_before));
		tuple.not_after = tuple.not_before;
	      }
	    else
	      ASSERT(spki_parse_valid(&iterator, &tuple));
	    	  
	    result = spki_5_tuple_list_filter(&db, result,
					      &tuple, filter_by_date);
	  }
	/* Done with the input file. */
	free(data);
      }

#if 0
      /* At least case 21 results in an empty acl list */
      ASSERT(result);
#endif

      /* The result should be an acl. */
      if (result)
	ASSERT(!result->car->issuer);
      
      spki_5_tuple_list_release(&db, result);

      spki_acl_clear(&db);
    }
}
