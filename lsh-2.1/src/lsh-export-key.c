/* lsh-export-key.c
 *
 * Reads an sexp in given form, and writes it in ssh2 form.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2000 Jean-Pierre Stierlin, Niels Möller
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

/* Test output:
[nisse@cuckoo src]$ ./lsh-export-key < testkey.pub 
---- BEGIN SSH2 PUBLIC KEY ----
Comment: "768-bit dsa"
AAAAB3NzaC1kc3MAAABhAJw3J7CMyAKiX8F1Mz1dNguVQi7VZQQrLl8DeWNQaSkqmIPjsc
zSn4Cjv9BOt8FM46AZbw+aSou0jpiFPJJiQjpT5U1ArPLoMqRpopqcZqcVubRKALTzytgw
vvXyoHb84wAAABUAmm14nnnHQtwx5ZUgRrjv98iv4KcAAABgENZmq1qm4jdJJB7IAC5Ecr
vcjhlACNcPD4UQ0Bgk66/MJOxvrwf0V+ZtTfb8ZaQlKdu84vB2VxVcB8zo0ds01I6eLG2f
/nDENvwp0TkNKf1uyEWPjNQGI/ImAqukiSWjAAAAYDe6o/C8faYCpuduLPQrl8Co6z7HgC
yIaRCzBjD8bY6L5qZp4G//8PVJVhxXh3vAS6LbgDCFoa2HZ1/vxHpML+gl3FPjAOxZPs27
B2CTISEmV3KYx5NJpyKC3IBw/ckP6Q==
---- END SSH2 PUBLIC KEY ----
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <fcntl.h>
/* #include <string.h> */
#include <unistd.h>

#include <nettle/base64.h>
#include <nettle/sexp.h>

#include "algorithms.h"
#include "alist.h"
#include "atoms.h"
#include "crypto.h"
#include "format.h"
#include "io.h"
#include "lsh.h"
#include "lsh_argp.h"
#include "lsh_string.h"
#include "spki.h"
#include "version.h"
#include "werror.h"
#include "xalloc.h"

enum output_mode
  {
    OUTPUT_STANDARD = 0,
    OUTPUT_FINGERPRINT = 1,
    OUTPUT_OPENSSH = 2,
  };

#include "lsh-export-key.c.x"

static struct lsh_string *
make_header(const char *name, const char *value)
{
  return value
    ? ssh_format("%lz: %lz\n", name, value)
    : ssh_format("");
}

#define BLOCKS 10
#define LINE_LENGTH (BASE64_TEXT_BLOCK_SIZE * BLOCKS)
#define BINARY_LENGTH (BASE64_BINARY_BLOCK_SIZE * BLOCKS)

/* Includes a newline at the end. */
static struct lsh_string *
encode_base64(const struct lsh_string *s)
{
  uint32_t input_length = lsh_string_length(s);
  const uint8_t *data = lsh_string_data(s);

  unsigned encoded_length = BASE64_ENCODE_RAW_LENGTH(input_length);
  unsigned lines = (encoded_length + LINE_LENGTH - 1) / LINE_LENGTH;
  struct base64_encode_ctx ctx;
  unsigned length = encoded_length + lines;
  unsigned line;
  unsigned final;
  unsigned out;
  
  struct lsh_string *res = lsh_string_alloc(length+3);

  base64_encode_init(&ctx);
  for (out = 0, line = 0; line + 1 < lines; line++)
    {
      lsh_string_base64_encode_update(res, out,
				      &ctx,
				      BINARY_LENGTH, data + line * BINARY_LENGTH);
      out += LINE_LENGTH;
      lsh_string_putc(res, out++, '\n');
    }
  final = lsh_string_base64_encode_update(res, out,
					  &ctx,
					  input_length - line * BINARY_LENGTH,
					  data + line * BINARY_LENGTH);
  final += lsh_string_base64_encode_final(res, out+final, &ctx);
  
  if (final)
    {
      out += final;
      lsh_string_putc(res, out++, '\n');
    }
  assert(out == length);
  lsh_string_trunc(res, length);
  
  lsh_string_free(s);
  
  return res;
}

static struct lsh_string *
encode_base64_line(const struct lsh_string *s)
{
  uint32_t input_length = lsh_string_length(s);
  const uint8_t *data = lsh_string_data(s);

  unsigned encoded_length = BASE64_ENCODE_RAW_LENGTH(input_length);
  struct base64_encode_ctx ctx;
  unsigned out;
  
  struct lsh_string *res = lsh_string_alloc(encoded_length + BASE64_ENCODE_FINAL_LENGTH);

  base64_encode_init(&ctx);
  out = lsh_string_base64_encode_update(res, 0,
					&ctx,
					input_length, data);
  out += lsh_string_base64_encode_final(res, out, &ctx);

  assert (out == encoded_length);
  lsh_string_trunc(res, out);
  
  lsh_string_free(s);
  
  return res;
}


static struct lsh_string *
sexp_to_ssh2_key(struct lsh_string *expr,
                 struct export_key_options *options)
{
  struct sexp_iterator i;
  struct verifier *v;
  int algorithm_name;
  struct lsh_string *key;
  
  if (!(lsh_string_transport_iterator_first(expr, &i)
	&& sexp_iterator_check_type(&i, "public-key")))
    {
      werror("Only conversion of public keys implemented.\n");
      return NULL;
    }

  v = spki_make_verifier(options->algorithms, &i, &algorithm_name);
  if (!v)
    {
      werror("Unsupported algorithm\n");
      return NULL;
    }

  key = PUBLIC_KEY(v);
  switch (options->mode)
    {
    default:
      fatal("Internal error.\n");

    case OUTPUT_STANDARD:
      return ssh_format("---- BEGIN SSH2 PUBLIC KEY ----\n"
			"%lfS"
			"%lfS"
			"\n%lfS"
			"---- END SSH2 PUBLIC KEY ----\n",
			make_header("Subject", options->subject),
			make_header("Comment", options->comment),
			encode_base64(key));

    case OUTPUT_FINGERPRINT:
      {
	struct lsh_string *output
	  = ssh_format("MD5 fingerprint: %lfS\n"
		       "Bubble Babble: %lfS\n",
		       lsh_string_colonize( 
			 ssh_format("%lfxS", 
				    hash_string(&crypto_md5_algorithm, key, 0)), 
			 2, 1),
		       lsh_string_bubblebabble( 
			 hash_string(&crypto_sha1_algorithm, key, 0), 1));
	lsh_string_free(key);
	return output;
      }

    case OUTPUT_OPENSSH:
      {
	key = encode_base64_line(key);
	switch (algorithm_name)
	  {
	  default:
	    werror("Keys of type %a not supported.\n");
	    lsh_string_free(key);
	    return NULL;
	    
	  case ATOM_RSA_PKCS1_SHA1:
	  case ATOM_RSA_PKCS1_MD5:
	  case ATOM_RSA_PKCS1:
	    return ssh_format("%la %lfS\n", ATOM_SSH_RSA, key);

	  case ATOM_DSA:
	    return ssh_format("%la %lfS\n", ATOM_SSH_DSS, key);
	  }
      }
    }			
}

/* Option parsing */

const char *argp_program_version
= "lsh-export-key-" VERSION;

const char *argp_program_bug_address = BUG_ADDRESS;

#define OPT_INFILE 'r'
#define OPT_OUTFILE 'o'
#define OPT_SUBJECT 's'
#define OPT_COMMENT 'c'
#define OPT_OPENSSH 0x100
#define OPT_FINGERPRINT 0x101

static const struct argp_option
main_options[] =
{
  /* Name, key, arg-name, flags, doc, group */
  { "input-file", OPT_INFILE, "Filename", 0, "Default is stdin", 0 },
  { "output-file", OPT_OUTFILE, "Filename", 0, "Default is stdout", 0 },
  { "subject", OPT_SUBJECT, "subject string", 0, "Add subject to output key.", 0 },
  { "comment", OPT_COMMENT, "comment string", 0, "Add comment to output key.", 0 },
  { "fingerprint", OPT_FINGERPRINT, NULL, 0, "Show key fingerprint.", 0 },
  { "openssh", OPT_OPENSSH, NULL, 0, "Output key in openssh single-line format.", 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* GABA:
(class
  (name export_key_options)
  (vars
    (algorithms object alist)
    (mode . "enum output_mode")
    (infile . "const char *")
    (outfile . "const char *")
    (subject . "const char *")
    (comment . "const char *")))
*/

static struct export_key_options *
make_options(void)
{
  NEW(export_key_options, self);
  self->infile = NULL;
  self->subject = NULL;
  self->comment = NULL;
  self->mode = OUTPUT_STANDARD;
  self->algorithms = all_signature_algorithms(NULL);

  return self;
}

static const struct argp_child
main_argp_children[] =
{
  { &werror_argp, 0, "", 0 },
  { NULL, 0, NULL, 0}
};

static error_t
main_argp_parser(int key, char *arg, struct argp_state *state)
{
  CAST(export_key_options, self, state->input);

  switch(key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    case ARGP_KEY_INIT:
      state->child_inputs[0] = NULL;
      break;
    case ARGP_KEY_END:
      break;
    case OPT_INFILE:
      self->infile = arg;
      break;
    case OPT_OUTFILE:
      self->outfile = arg;
      break;
    case OPT_SUBJECT:
      self->subject = arg;
      break;
    case OPT_COMMENT:
      self->comment = arg;
      break;
    case OPT_FINGERPRINT:
      self->mode = OUTPUT_FINGERPRINT;
      break;
    case OPT_OPENSSH:
      self->mode = OUTPUT_OPENSSH;
      break;
    }
  
  return 0;
}

static const struct argp
main_argp =
{ main_options, main_argp_parser, NULL,
  "Reads an s-expression on stdin, and outputs the same "
  "s-expression on stdout, using OpenSSH/SSH2 encoding format.",
  main_argp_children,
  NULL, NULL
};
  

#define SEXP_BUFFER_SIZE 1024

#ifdef MACOS
char *applname = "lsh-export-key";
//char *defargstr = "-r identity.pub";
char *defargstr = "";
int appl_main(int argc, char **argv);
#define main appl_main
#endif

#define MAX_KEY_SIZE 10000

int main(int argc, char **argv)
{
  struct export_key_options *options = make_options();

  const struct exception *e;
  int in = STDIN_FILENO;
  int out = STDOUT_FILENO;
  
  struct lsh_string *input;
  struct lsh_string *output;
    
  argp_parse(&main_argp, argc, argv, 0, NULL, options);

  if (options->infile)
    {
      in = open(options->infile, O_RDONLY);
      if (in < 0)
	{
	  werror("Failed to open '%z' for reading %e\n",
		 options->infile, errno);
	  return EXIT_FAILURE;
	}
    }
  
  if (options->outfile)
    {
      out = open(options->outfile,
                 O_WRONLY | O_CREAT, 0666);
      if (out < 0)
        {
	  werror("Failed to open '%z' for writing %e\n",
		 options->outfile, errno);
          return EXIT_FAILURE;
        }
    }

  /* Guess size 5000 */
  input = io_read_file_raw(in, 5000);
  
  if (!input)
    {
      werror("Failed to read '%z' %e\n",
             options->infile, errno);
      return EXIT_FAILURE;
    }

  output = sexp_to_ssh2_key(input, options);
  lsh_string_free(input);
  
  if (!output)
    return EXIT_FAILURE;

  e = write_raw(out, STRING_LD(output));
  lsh_string_free(output);

  if (e)
    {
      werror("%z\n", e->msg);
      return EXIT_FAILURE;
    }

  gc_final();
  
  return EXIT_SUCCESS;
}
  
