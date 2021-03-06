/* translate_signal.c
 *
 * Translate local signal numbers to canonical numbers, and vice versa.
 * The value of "canonical" is rather arbitrary.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998 Niels M�ller
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>

#include "translate_signal.h"

#include "atoms.h"

struct signal_assoc
{
  int network;
  int local;
};

/* IOT is the mnemonic for the PDP-11 instruction used by the abort function */
#ifdef SIGIOT
# ifndef SIGABRT
#  define SIGABRT SIGIOT
# endif
#endif

/* The body is generated by
 *
 SIGNALS="ABRT ALRM FPE HUP ILL INT KILL PIPE QUIT SEGV TERM USR1 USR2"
 for s in $SIGNALS; do
   echo "#ifdef SIG""$s"
   echo "  { ATOM_""$s, SIG""$s"},
   echo "#endif"
 done 
*/

static const struct signal_assoc translate[] =
{
#ifdef SIGABRT
  { ATOM_ABRT, SIGABRT},
#endif
#ifdef SIGALRM
  { ATOM_ALRM, SIGALRM},
#endif
#ifdef SIGFPE
  { ATOM_FPE, SIGFPE},
#endif
#ifdef SIGHUP
  { ATOM_HUP, SIGHUP},
#endif
#ifdef SIGILL
  { ATOM_ILL, SIGILL},
#endif
#ifdef SIGINT
  { ATOM_INT, SIGINT},
#endif
#ifdef SIGKILL
  { ATOM_KILL, SIGKILL},
#endif
#ifdef SIGPIPE
  { ATOM_PIPE, SIGPIPE},
#endif
#ifdef SIGQUIT
  { ATOM_QUIT, SIGQUIT},
#endif
#ifdef SIGSEGV
  { ATOM_SEGV, SIGSEGV},
#endif
#ifdef SIGTERM
  { ATOM_TERM, SIGTERM},
#endif
#ifdef SIGUSR1
  { ATOM_USR1, SIGUSR1},
#endif
#ifdef SIGUSR2
  { ATOM_USR2, SIGUSR2},
#endif
  { -1, -1 }
};
  
/* Returns ATOM_SIGNAL_UNKNOWN_LOCAL if the signal has no network 
 * number */
int signal_local_to_network(int signal)
{
  int i;

  if (!signal)
    return 0;
  
  for (i = 0; ; i++)
    {
      if (translate[i].local < 0)
	return ATOM_SIGNAL_UNKNOWN_LOCAL;
      if (translate[i].local == signal)
	return translate[i].network;
    }
}

/* Returns 0 if the signal has no local number */
int signal_network_to_local(int signal)
{
  int i;

  if (!signal)
    return 0;

  for (i = 0; ; i++)
    {
      if (translate[i].local < 0)
	return 0;
      if (translate[i].network == signal)
	return translate[i].local;
    }
}
