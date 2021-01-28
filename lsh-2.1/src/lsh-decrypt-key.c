/* lsh-decrypt-key.c
 *
 * Reads a (private) key on stdin, decrypts it and writes it on stdout.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998, 2003 Niels Möller, Pontus Sköld
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

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

#include <nettle/sexp.h>

#include "lsh-decrypt-key.c.x"

/* Option parsing */

const char *argp_program_version
= "lsh-writekey-" VERSION;

const char *argp_program_bug_address = BUG_ADDRESS;

#define OPT_INFILE 0x201
#define OPT_OUTFILE 0x202
#define OPT_ASKPASS 0x203

/* GABA:
   (class
     (name lsh_decryptkey_options)
     (vars
       ; Base filename
       (tty object interact)
       (in_fd . int) 
       (out_fd . int)))
*/

static struct lsh_decryptkey_options *
make_lsh_decryptkey_options(void)
{
  NEW(lsh_decryptkey_options, self);
  
  /* We don't need window change tracking. */
  self->tty = make_unix_interact();

  self->in_fd = 0; /* stdin */
  self->out_fd = 1; /* stdout */

  return self;
}

static const struct argp_option
main_options[] =
{
  /* Name, key, arg-name, flags, doc, group */
  { "in", OPT_INFILE, "encrypted key filename", 0, "(defaults to stdin)", 0},
  { "out", OPT_OUTFILE, "decrypted key filename", 0, "(defaults to stdout)", 0},

  { "askpass", OPT_ASKPASS, "Program", 0,
    "Program to use for reading passwords. "
    "Should be an absolute filename.", 0 },

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
  CAST(lsh_decryptkey_options, self, state->input);

  int i;

  switch(key)
    {
    default:
      return ARGP_ERR_UNKNOWN;

    case ARGP_KEY_INIT:
      state->child_inputs[0] = NULL;
      break;

    case OPT_ASKPASS:
      INTERACT_SET_ASKPASS(self->tty, arg);
      break;

    case OPT_INFILE:
      i = open(arg, O_RDONLY);
      if (i<0)
	{
	  perror( "Failed to open key for input");
	  exit( EXIT_FAILURE );
	}
      else
	self->in_fd = i;
      break;

    case OPT_OUTFILE:
      i = open(arg, O_WRONLY | O_CREAT | O_EXCL, 0600 );
      if (i<0)
	{
	  perror( "Failed to open key for output " 
		  "(note that existing files aren't\n"
		  "overwritten - move them out of the way first)");
          exit( EXIT_FAILURE );
	}
      else
	self->out_fd = i;
      break;

    case ARGP_KEY_END:      
      break;
    }
  return 0;
}

static const struct argp
main_argp =
{ main_options, main_argp_parser, 
  NULL,
  ( "Decrypts the key given and outputs the decrypted key."
    ),
  main_argp_children, 
  NULL, NULL
};

int
main(int argc, char **argv)
{
  struct lsh_decryptkey_options *options = make_lsh_decryptkey_options();
  struct lsh_string *input;
  struct lsh_string *output;
  const struct exception *e;
  struct alist *mac = make_alist(0, -1);
  struct alist *crypto = make_alist(0, -1);
  
  argp_parse(&main_argp, argc, argv, 0, NULL, options);

  input = io_read_file_raw(options->in_fd, 2000);

  if (!input)
    {
      werror("Failed to read key %e\n", errno);
      return EXIT_FAILURE;
    }
  
  
  
  alist_select_l(mac, all_symmetric_algorithms(),
		 2, ATOM_HMAC_SHA1, ATOM_HMAC_MD5, -1);
  alist_select_l(crypto, all_symmetric_algorithms(),
		 4, ATOM_3DES_CBC, ATOM_BLOWFISH_CBC,
		 ATOM_TWOFISH_CBC, ATOM_AES256_CBC, -1);
  
  output = spki_pkcs5_decrypt(mac, 
			      crypto,
			      options->tty,
			      input);
  if (!output)
    {
      werror("Decrypting private key failed.\n");
      return EXIT_FAILURE;
    }
    
  e = write_raw(options->out_fd, STRING_LD(output));
  lsh_string_free(output);
  
  if (e)
    {
      werror("Writing decrypted key failed: %z\n",
             e->msg);
      return EXIT_FAILURE;
    }
  
  gc_final();
  
  return EXIT_SUCCESS;
}
