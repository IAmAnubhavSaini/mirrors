/* lsh-writekey.c
 *
 * Reads a (private) key on stdin, and saves it a private and a public file.
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>

#include <fcntl.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <nettle/sexp.h>

#include "algorithms.h"
#include "crypto.h"
#include "environ.h"
#include "format.h"
#include "io_commands.h"
#include "interact.h"
#include "lsh_string.h"
#include "publickey_crypto.h"
#include "spki.h"
#include "version.h"
#include "werror.h"
#include "xalloc.h"

#include "lsh-writekey.c.x"

/* Option parsing */

const char *argp_program_version
= "lsh-writekey-" VERSION;

const char *argp_program_bug_address = BUG_ADDRESS;

#define OPT_SERVER 0x200

/* GABA:
   (class
     (name lsh_writekey_options)
     (vars
       ; Base filename
       (public_file string)
       (private_file string)

       (server . int)
       (tty object interact)
       
       (label string)
       (passphrase string)

       (crypto_algorithms object alist)
       (signature_algorithms object alist)
       ; We use this only for salt and iv generation.
       (r object randomness)

       ; Zero means default, which depends on the --server flag.
       (crypto_name . int)
       (crypto object crypto_algorithm)
       (iterations . uint32_t)))
*/

static struct lsh_writekey_options *
make_lsh_writekey_options(void)
{
  NEW(lsh_writekey_options, self);
  self->public_file = NULL;
  self->private_file = NULL;
  self->server = 0;
  
  /* We don't need window change tracking. */
  self->tty = make_unix_interact();
    
  self->label = NULL;

  self->passphrase = NULL;
  self->iterations = 1500;

  self->crypto_algorithms = all_symmetric_algorithms();

  /* NOTE: We don't need any randomness here, as we won't be signing
   * anything. */
  self->signature_algorithms = all_signature_algorithms(NULL);

  self->r = NULL;
  
  self->crypto_name = 0;
  self->crypto = NULL;
  
  return self;
}

static const struct argp_option
main_options[] =
{
  /* Name, key, arg-name, flags, doc, group */
  { "output-file", 'o', "Filename", 0, "Default is ~/.lsh/identity", 0 },
  { "server", OPT_SERVER, NULL, 0,
    "Use the server's seed-file, and change the default output file "
    "to /etc/lsh_host_key'.", 0 },
  { "iteration-count", 'i', "PKCS#5 iteration count", 0, "Default is 1500", 0 },
  { "crypto", 'c', "Algorithm", 0, "Encryption algorithm for the private key file.", 0 },
  { "label", 'l', "Text", 0, "Unencrypted label for the key.", 0 },
  { "passphrase", 'p', "Password", 0, NULL, 0 },
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
  CAST(lsh_writekey_options, self, state->input);

  switch(key)
    {
    default:
      return ARGP_ERR_UNKNOWN;

    case ARGP_KEY_INIT:
      state->child_inputs[0] = NULL;
      break;

    case ARGP_KEY_END:
      if (!self->private_file)
	{
	  if (self->server)
	    self->private_file = make_string("/etc/lsh_host_key");
	  else
	    {
	      char *home = getenv(ENV_HOME);
	  
	      if (!home)
		{
		  argp_failure(state, EXIT_FAILURE, 0, "$HOME not set.");
		  return EINVAL;
		}
	      else
		{
		  struct lsh_string *s = ssh_format("%lz/.lsh", home);
		  const char *cs = lsh_get_cstring(s);
		  if (mkdir(cs, 0755) < 0)
		    {
		      if (errno != EEXIST)
			argp_failure(state, EXIT_FAILURE, errno,
				     "Creating directory %s failed.", cs);
		    }
		  lsh_string_free(s);
		  self->private_file = ssh_format("%lz/.lsh/identity", home);
		}
	    }
	}
      self->public_file = ssh_format("%lS.pub", self->private_file);

      /* Default behaviour is to encrypt the key unless running in
	 server mode. */
      if (!self->crypto_name && !self->server)
	{
	  self->crypto_name = ATOM_AES256_CBC;
	  self->crypto = &crypto_aes256_cbc_algorithm;	 
	}
      if (self->crypto)
	{
	  if (!self->label)
	    {
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 300
#endif
	      char host[MAXHOSTNAMELEN];
	      const char *name;
	  
	      USER_NAME_FROM_ENV(name);

	      if (!name)
		{
		  argp_failure(state, EXIT_FAILURE, 0,
			       "LOGNAME not set. Please use the -l option.");
		  return EINVAL;
		}

	      if ( (gethostname(host, sizeof(host)) < 0)
		   && (errno != ENAMETOOLONG) )
		argp_failure(state, EXIT_FAILURE, errno,
			     "Can't get the host name. Please use the -l option.");
	      
	      self->label = ssh_format("%lz@%lz", name, host);
	    }
	  self->r = (self->server
		     ? make_system_random()
		     : make_user_random(getenv(ENV_HOME)));
	  if (!self->r)
	    argp_failure(state, EXIT_FAILURE, 0, 
			 "Failed to initialize randomness generator.");
	}
      break;
      
    case 'o':
      self->private_file = make_string(arg);
      break;

    case OPT_SERVER:
      self->server = 1;      
      break;
      
    case 'i':
      {
	long i;
	char *end;
	i = strtol(arg, &end, 0);

	if ((end == arg) || *end || (i < 1))
	  {
	    argp_failure(state, EXIT_FAILURE, 0, "Invalid iteration count.");
	    return EINVAL;
	  }
	else if (i > PKCS5_MAX_ITERATIONS)
	  {
	    argp_error(state, "Iteration count ridiculously large (> %d).",
		       PKCS5_MAX_ITERATIONS);
	    return EINVAL;
	  }
	else
	  self->iterations = i;

	break;
      }

    case 'c':
      {
	int name = lookup_crypto(self->crypto_algorithms, arg, &self->crypto);

	if (name)
	  self->crypto_name = name;
	else
	  {
	    list_crypto_algorithms(state, self->crypto_algorithms);
	    argp_error(state, "Unknown crypto algorithm '%s'.", arg);
	  }
	break;
      }
      
    case 'l':
      self->label = ssh_format("%lz", arg);
      break;
      
    case 'p':
      self->passphrase = ssh_format("%lz", arg);
      break;
    }
  return 0;
}

static const struct argp
main_argp =
{ main_options, main_argp_parser, 
  NULL,
  ( "Splits a keypair in one private and one public file, "
    "optionally encrypting the private file using a passphrase.\v"
    "Common usage is to pipe the output from lsh-keygen into this program."
    ),
  main_argp_children,
  NULL, NULL
};

static int
file_exists(const struct lsh_string *file)
{
  struct stat sbuf;
  return (stat(lsh_get_cstring(file), &sbuf) == 0
	  || errno != ENOENT);
}

/* Returns 1 for success, 0 for errors. */
static int
check_file(const struct lsh_string *file)
{
  if (file_exists(file))
    {
      werror("File `%S' already exists.\n"
	     "lsh-writekey doesn't overwrite existing key files.\n"
	     "If you *really* want to do that, you should delete\n"
	     "the existing files first\n",
	     file);
      return 0;
    }
  return 1;
}

static int
open_file(const struct lsh_string *file)
{
  int fd = open(lsh_get_cstring(file),
                O_CREAT | O_EXCL | O_WRONLY,
                0600);

  if (fd < 0)
    werror("Failed to open `%S'for writing %e\n", file, errno);

  return fd;
}

static struct lsh_string *
process_private(const struct lsh_string *key,
                struct lsh_writekey_options *options)
{
  if (options->crypto)
    {
      CAST_SUBTYPE(mac_algorithm, hmac,
                   ALIST_GET(options->crypto_algorithms, ATOM_HMAC_SHA1));
      assert(hmac);
      
      while (!options->passphrase)
	{
	  struct lsh_string *pw;
	  struct lsh_string *again;
	  
	  pw = INTERACT_READ_PASSWORD(options->tty, 500,
				      ssh_format("Enter new passphrase: "));
	  if (!pw)
	    {
	      werror("Aborted.");
	      return NULL;
	    }

	  
	  again = INTERACT_READ_PASSWORD(options->tty, 500,
					 ssh_format("Again: "));
	  if (!again)
	    {
	      werror("Aborted.");
	      lsh_string_free(pw);
	      return NULL;
	    }

	  if (lsh_string_eq(pw, again))
	    options->passphrase = pw;
	  else
	    lsh_string_free(pw);
	  
	  lsh_string_free(again);
	}

      return spki_pkcs5_encrypt(options->r,
				options->label,
				ATOM_HMAC_SHA1,
				hmac,
				options->crypto_name,
				options->crypto,
				10, /* Salt length */
				options->passphrase,
				options->iterations,
				key);
    }
  else
    return lsh_string_dup(key);
}

static struct lsh_string *
process_public(const struct lsh_string *key,
               struct lsh_writekey_options *options)
{
  struct signer *s;
  struct verifier *v;
  
  s = spki_make_signer(options->signature_algorithms, key, NULL);
  
  if (!s)
    return NULL;

  v = SIGNER_GET_VERIFIER(s);
  assert(v);

  return PUBLIC_SPKI_KEY(v, 1);
}

int
main(int argc, char **argv)
{
  struct lsh_writekey_options *options = make_lsh_writekey_options();
  int private_fd;
  int public_fd;
  struct lsh_string *input;
  struct lsh_string *output;
  const struct exception *e;

  argp_parse(&main_argp, argc, argv, 0, NULL, options);

  if (! (check_file(options->private_file)
	 && check_file(options->public_file)))
    return EXIT_FAILURE;
  
  input = io_read_file_raw(STDIN_FILENO, 2000);

  if (!input)
    {
      werror("Failed to read key from stdin %e\n", errno);
      return EXIT_FAILURE;
    }
  
  if (!lsh_string_length(input))
    {
      werror("Empty key on input, giving up.\n");
      return EXIT_FAILURE;
    }
  
  output = process_private(input, options);
  if (!output)
    return EXIT_FAILURE;

  private_fd = open_file(options->private_file);
  if (private_fd < 0)
    return EXIT_FAILURE;

  e = write_raw(private_fd, STRING_LD(output));
  lsh_string_free(output);

  if (e)
    {
      werror("Writing private key failed: %z\n",
             e->msg);
      return EXIT_FAILURE;
    }

  output = process_public(input, options);
  lsh_string_free(input);
  
  if (!output)
    return EXIT_FAILURE;

  public_fd = open_file(options->public_file);
  if (public_fd < 0)
    return EXIT_FAILURE;
  
  e = write_raw(public_fd, STRING_LD(output));
  lsh_string_free(output);
  
  if (e)
    {
      werror("Writing public key failed: %z\n",
             e->msg);
      return EXIT_FAILURE;
    }
  
  gc_final();
  
  return EXIT_SUCCESS;
}
