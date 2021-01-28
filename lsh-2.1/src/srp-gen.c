/* srp-gen.c
 *
 * Create an SRP verifier
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2000 Niels Möller
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

#include <assert.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "srp.h"

#include "crypto.h"
#include "environ.h"
#include "format.h"
#include "interact.h"
#include "io.h"
#include "lsh_string.h"
#include "randomness.h"
#include "srp.h"
#include "version.h"
#include "werror.h"
#include "xalloc.h"

#define BLOCK_SIZE 2000
#define SALT_SIZE 20

#include "srp-gen.c.x"

/* Option parsing */

const char *argp_program_version
= "srp-gen-" VERSION;

const char *argp_program_bug_address = BUG_ADDRESS;

/* GABA:
   (class
     (name srp_gen_options)
     (vars
       (tty object interact)
       
       (e object exception_handler)
       (G const object zn_group)
       (H const object hash_algorithm)
       
       (file string)
       (dest object abstract_write)

       (name . "const char *")
       (passwd string)
       (r object randomness)))
*/

static struct srp_gen_options *
make_srp_gen_options(struct exception_handler *e)
{
  NEW(srp_gen_options, self);

  /* We don't need window change tracking. */
  self->tty = make_unix_interact();
  
  self->e = e;

  self->G = make_ssh_ring_srp_1();
  self->H = &crypto_sha1_algorithm;
  self->file = NULL;
  self->dest = NULL;

  USER_NAME_FROM_ENV(self->name);
  self->passwd = NULL;

  /* We use this only for generating the salt. */
  self->r = make_user_random(getenv(ENV_HOME));

  return self;
}

static const struct argp_option
main_options[] =
{
  /* Name, key, arg-name, flags, doc, group */
  { "user", 'l', "User name", 0, NULL, 0 },
  { "password", 'p', "Password", 0, NULL, 0 },
  { "output-file", 'o', "Filename", 0, "Default is to write to stdout.", 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static const struct argp_child
main_argp_children[] =
{
  { &werror_argp, 0, "", 0 },
  { NULL, 0, NULL, 0}
};

static error_t
main_argp_parser(int key, char *arg, struct argp_state *state)
{
  CAST(srp_gen_options, self, state->input);

  switch(key)
    {
    default:
      return ARGP_ERR_UNKNOWN;

    case ARGP_KEY_INIT:
      state->child_inputs[0] = NULL;
      break;

    case ARGP_KEY_END:
      if (!self->name)
	argp_error(state, "No user name given. Use the -l option, or set LOGNAME in the environment.");

      {
	if (self->file)
	  {
	    const char *cfile = lsh_get_cstring(self->file);

	    self->dest = io_write_file(cfile,
				       O_CREAT | O_EXCL | O_WRONLY,
				       0600, self->e);
	    if (!self->dest)
	      argp_failure(state, EXIT_FAILURE, errno,
			   "Could not open '%s'.", cfile);
	  }
	else
	  {
	    self->dest = make_io_write_file(STDOUT_FILENO, self->e);
	  }
      }
      
      while (!self->passwd)
	{
	  struct lsh_string *pw;
	  struct lsh_string *again;

	  pw = INTERACT_READ_PASSWORD(self->tty, 500,
				      ssh_format("Enter new SRP password: "));
	  if (!pw)
	    argp_failure(state, EXIT_FAILURE, 0, "Aborted.");

	  again = INTERACT_READ_PASSWORD(self->tty, 500,
					 ssh_format("Again: "));
	  if (!again)
	    argp_failure(state, EXIT_FAILURE, 0, "Aborted.");

	  if (lsh_string_eq(pw, again))
	    self->passwd = pw;
	  else
	    lsh_string_free(pw);

	  lsh_string_free(again);
	}
      
      break;

    case 'o':
      self->file = make_string(arg);
      break;

    case 'p':
      self->passwd = ssh_format("%lz", arg);
      break;
    }
  return 0;
}

static const struct argp
main_argp =
{ main_options, main_argp_parser, 
  NULL,
  "Generates a password verifier for the Secure Remote Password protocol.",
  main_argp_children,
  NULL, NULL
};

static struct lsh_string *
srp_gen(struct srp_gen_options *options)
{
  struct lsh_string *salt;
  struct lsh_string *name;
  struct lsh_string *res;

  /* NOTE: Allows random to be of bad quality */
  salt = lsh_string_random(options->r, SALT_SIZE);

  name = ssh_format("%lz", options->name);

  /* FIXME: Leaks some strings. */
  res = srp_make_verifier(options->G, options->H,
			  salt, name, options->passwd);
  lsh_string_free(name);

  return res;
}

static void
do_srp_gen_handler(struct exception_handler *s UNUSED,
			const struct exception *e)
{
  werror("lsh_writekey: %z\n", e->msg);

  exit(EXIT_FAILURE);
}

static struct exception_handler exc_handler =
STATIC_EXCEPTION_HANDLER(do_srp_gen_handler, NULL);

int main(int argc, char **argv)
{
  struct srp_gen_options *options
    = make_srp_gen_options(&exc_handler);

  io_init();
  
  argp_parse(&main_argp, argc, argv, 0, NULL, options);

  /* FIXME: Use write_raw instead. */
  A_WRITE(options->dest, srp_gen(options));
  
  io_run();
  io_final();
  
  return EXIT_SUCCESS;
}
