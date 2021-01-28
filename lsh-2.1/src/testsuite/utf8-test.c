#include "testutils.h"
#include "charset.h"

/* Uses Markus Kuhn's test file as input. Originally at
   http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt. */

#define VERBOSE 0

static FILE *
open_input(void)
{
  FILE *f;
  char *fname;
  const char *srcdir = getenv("srcdir");
  
  if (!srcdir)
    {
      fprintf(stderr, "$srcdir not set in the environment.\n");
      FAIL();
    }
  fname = alloca(strlen(srcdir) + 25);
  sprintf(fname, "%s/%s", srcdir, "UTF-8-test.txt");

  f = fopen(fname, "rb");
  if (!f)
    {
      fprintf(stderr, "Could not open %s.\n", fname);
      FAIL();
    }
  return f;
}

static int
get_line(FILE *f, uint32_t size, char *buffer)
{
  uint32_t length;
  if (!fgets(buffer, size, f))
    {
      return -1;
    }

  length = strlen(buffer);
  if (length > 0 && buffer[length - 1] == '\n')
    buffer[--length] = 0;

  return length;
}

#define LINE_LENGTH 79

int
test_main(void)
{
  FILE *f = open_input();
  char buffer[200];
  struct lsh_string *s;
  int length;
  int lineno = 0;
  
  /* This test doesn't really check that the decoder works right, it only
     checks that it doesn't crash. */
  while ( (length = get_line(f, sizeof(buffer), buffer)) >= 0)
    {
      static const int charsets[] =
	{ CHARSET_UTF8, CHARSET_LATIN1, CHARSET_USASCII, -1 };
      static const int flags[] =
	{
	  0, utf8_tolerant | utf8_replace, utf8_replace, utf8_paranoid,
	  utf8_paranoid | utf8_replace, -1
	};
      int i, j;
      
      /* Skip description lines */
      if (length < LINE_LENGTH)
	continue;

      lineno++;
      
      for (i = 0; charsets[i] >= 0; i++)
	{
	  set_local_charset(charsets[i]);
	  for (j = 0; flags[j] >= 0; j++)
	    {
	      s = low_utf8_to_local(length, buffer, flags[j]);
#if VERBOSE
	      if (!s)
		fprintf(stderr, "!%s\n", buffer);
	      else
		fprintf(stderr, " %s\n", lsh_string_data(s));
#endif
	      if (flags[j] == (utf8_tolerant | utf8_replace) && !s)
		{
		  fprintf(stderr, "utf8_to_local failed on line %d:\n"
			  "`%s'", lineno, buffer); 
		  FAIL();
		}
	      if (flags[j] == (utf8_replace | utf8_tolerant)
		  && charsets[i] != CHARSET_UTF8
		  && (!s || lsh_string_length(s) != LINE_LENGTH))
		{
		  fprintf(stderr, "Bad chracter count from utf8_to_local, line %d:\n"
			  "in:  %s\n"
			  "out: %s\n", lineno, buffer, lsh_string_data(s)); 
		  FAIL();
		}
	      lsh_string_free(s);
	    }
	}

    }

  if (ferror(f))
    FAIL();

  fclose(f);
  SUCCESS();
}
  
