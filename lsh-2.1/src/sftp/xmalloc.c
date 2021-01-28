/* xmalloc.c
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "xmalloc.h"

void *
xmalloc(size_t size)
{
  void *p = malloc(size);
  if (!p)
    {
      fprintf(stderr, "Virtual memory exhausted\n");
      abort();
    }
  return p;
}

char *
xstrdup(const char *s)
{
  size_t length = strlen(s);
  char *p = xmalloc(length + 1);
  memcpy(p, s, length);
  p[length] = '\0';

  return p;
}

void *
xrealloc(void *old, size_t size)
{
  void *p = realloc(old, size);
  if (!p)
    {
      fprintf(stderr, "Virtual memory exhausted\n");
      abort();
    }
  return p;
}
