/* userauth.h
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998 Niels Möller
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

#ifndef LSH_USERAUTH_H_INCLUDED
#define LSH_USERAUTH_H_INCLUDED

/* For uid_t */
#include <unistd.h>

#include "exception.h"
#include "resource.h"

/* Forward declaration */
struct env_value;
struct spawn_info;

#define GABA_DECLARE
#include "userauth.h.x"
#undef GABA_DECLARE

/* GABA:
   (class
     (name userauth_special_exception)
     (super exception)
     (vars
       (reply string)))
*/

/* FIXME: Perhaps it's better to use a const char * for the value? */
struct env_value
{
  const char *name;
  struct lsh_string *value;
};

struct exception *
make_userauth_special_exception(struct lsh_string *reply,
				const char *msg);

/* GABA:
   (class
     (name lsh_process)
     (super resource)
     (vars
       (signal method int int)))
*/

#define SIGNAL_PROCESS(p, s) ((p)->signal((p), (s)))

struct spawn_info
{
  /* For logging */
  struct address_info *peer;
  /* If a pty should be allocated */
  struct pty_info *pty;

  /* Is it a login session? */
  int login ;
  
  /* {in|out|err}[0] is for reading,
   * {in|out|err}[1] for writing. */

  /* Negative values for the child fd:s means that the slave tty should
   * be used. */
  int in[2]; int out[2]; int err[2];

  /* These are the arguments to the shell, not including
   * the traditional argv[0]. */
  unsigned argc;
  const char **argv;
  
  /* Currently, this environment is used also for the
   * execution of the lsh-execuv program, so dangerous
   * variables must not be set. */
  unsigned env_length;
  const struct env_value *env;
};

/* GABA:
   (class
     (name lsh_user)
     (vars
       ; This string include a terminating NUL-character, for
       ; compatibility with library and system calls.
       (name string)
       ; This doesn't really belong here, but there are a few
       ; functions that need it. 
       (uid . uid_t)
       
       ; Verify a password. Consumes the password string.
       (verify_password method void
                        "struct lsh_string *pw"
                        "struct command_continuation *c"
			"struct exception_handler *e")

       ; Check if a file in the user's home directory exists.
       ; Used by the current publickey userauth method.
       (file_exists method int "struct lsh_string *name" "int free")

       ; Open a file in the user's "~/.lsh" directory. File must be
       ; owned by the user and not writable for other users. If SECRET is 1,
       ; it must also not be readable by other users.
       (read_file method "const struct exception *"
                  "const char *name" "int secret"
		  "uint32_t limit"
		  ; Gets the contents of the file
		  "struct abstract_write *c")
		  
       ; Spawns a user process executing the user's login shell.
       ; Also closes the appropriate fd:s in info. '
       (spawn method "struct lsh_process *"
              "struct spawn_info *info"
              "struct exit_callback *c")))
*/

#define USER_VERIFY_PASSWORD(u, p, c, e) ((u)->verify_password((u), (p), (c), (e)))
#define USER_FILE_EXISTS(u, n, f) ((u)->file_exists((u), (n), (f)))
#define USER_READ_FILE(u, n, s, l, c) ((u)->read_file((u), (n), (s), (l), (c)))
#define USER_SPAWN(u, i, c) ((u)->spawn((u), (i), (c)))

/* This prototype doesn't really belong here. */
struct lsh_process *
unix_process_setup(pid_t pid,
		   struct lsh_user *user,
		   struct exit_callback **c,
		   struct address_info *peer,
		   struct lsh_string *tty);

#endif /* LSH_USERAUTH_H_INCLUDED */
