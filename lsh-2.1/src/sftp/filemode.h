/* Always use prototypes. I'm not sure how the below __STDC__ tests
   are supposed to work, but the default mode of operation for tru64
   cc is to ANSI-C, and it defines __STDC__ with the value 0. */
#ifndef PROTOTYPES
# define PROTOTYPES 1
#endif

#ifndef PARAMS
# if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

#include <sys/types.h>
#include <sys/stat.h>

#define MODE_STRING_LENGTH 10
void mode_string PARAMS ((short unsigned int mode, char *str));
void filemodestring PARAMS ((struct stat *statp, char *str));
