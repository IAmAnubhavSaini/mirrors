/* werror.c
 *
 * Simple diagnostics routines.
 *
 * $id$
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "werror.h"

void
werror(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  
  fprintf(stderr, "%s: ", werror_program_name);
  vfprintf(stderr, format, args);

  va_end(args);
}

void
fatal(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  
  fprintf(stderr, "%s: ", werror_program_name);
  vfprintf(stderr, format, args);

  va_end(args);

  exit(EXIT_FAILURE);
}

void
_fatal(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  
  fprintf(stderr, "%s: ", werror_program_name);
  vfprintf(stderr, format, args);

  va_end(args);

  _exit(EXIT_FAILURE);
}
