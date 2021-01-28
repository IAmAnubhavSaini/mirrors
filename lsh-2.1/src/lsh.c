/* lsh.c
 *
 * Client main program.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998, 1999, 2000 Niels Möller
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
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <nettle/sexp.h>

/* For struct spki_iterator */
#include "spki/parse.h"

#include "algorithms.h"
#include "alist.h"
#include "atoms.h"
#include "channel.h"
#include "charset.h"
#include "client.h"
#include "client_keyexchange.h"
#include "client_userauth.h"
#include "compress.h"
#include "connection_commands.h"
#include "crypto.h"
#include "environ.h"
#include "format.h"
#include "interact.h"
#include "io.h"
#include "io_commands.h"
#include "gateway.h"
#include "gateway_commands.h"
#include "handshake.h"
#include "lookup_verifier.h"
#include "lsh_string.h"
#include "publickey_crypto.h"
#include "randomness.h"
#include "sexp.h"
#include "spki.h"
#include "srp.h" 
#include "ssh.h"
#include "tcpforward.h"
#include "version.h"
#include "werror.h"
#include "xalloc.h"


#include "lsh_argp.h"

/* Forward declarations */

static struct request_service request_userauth_service =
STATIC_REQUEST_SERVICE(ATOM_SSH_USERAUTH);
#define REQUEST_USERAUTH_SERVICE (&request_userauth_service.super.super)

#include "lsh.c.x"

/* Block size for stdout and stderr buffers */
#define BLOCK_SIZE 32768

/* Window size for the session channel
 *
 * NOTE: Large windows seem to trig a bug in sshd2. */
#define WINDOW_SIZE 10000

/* GABA:
   (class
     (name lsh_options)
     (super client_options)
     (vars
       (algorithms object algorithms_options)

       (signature_algorithms object alist)
       (home . "const char *")
       
       (identity . "char *")
       (with_publickey . int)

       (with_srp_keyexchange . int)
       (with_dh_keyexchange . int)

       (kex_algorithms object int_list)
       
       (sloppy . int)
       (capture . "const char *")
       (capture_file object abstract_write)

       (known_hosts . "const char *")
       
       (start_gateway . int)))
*/


static struct lsh_options *
make_options(struct exception_handler *handler,
	     int *exit_code)
{
  NEW(lsh_options, self);
  const char *home = getenv(ENV_HOME);
  struct randomness *r = make_user_random(home);

  init_client_options(&self->super, r,
		      handler, exit_code);

  self->algorithms = make_algorithms_options(all_symmetric_algorithms());
  self->home = home;

  /* OK to init with NULL */  
  self->signature_algorithms = all_signature_algorithms(r);

  self->sloppy = 0;
  self->capture = NULL;
  self->capture_file = NULL;

  self->known_hosts = NULL;
  
  self->start_gateway = 0;

  self->with_publickey = 1;

  self->with_srp_keyexchange = 0;

  /* By default, enable only one of dh and srp. */
  self->with_dh_keyexchange = -1;
  
  self->kex_algorithms = NULL;
  
  return self;
}


/* Open hostkey database. By default, "~/.lsh/host-acls". */

static struct spki_context *
read_known_hosts(struct lsh_options *options)
{
  struct lsh_string *tmp = NULL;
  const char *s;
  struct lsh_string *contents;
  int fd;
  struct spki_iterator i;
  struct spki_context *context;
  const char *sexp_conv = getenv(ENV_SEXP_CONV);
  const char *args[] = { "sexp-conv", "-s", "canonical", NULL };
  
  context = make_spki_context(options->signature_algorithms);
  
  if (options->known_hosts)
    {
      s = options->known_hosts;
      fd = open(s, O_RDONLY);
    }
  else
    {
      tmp = ssh_format("%lz/.lsh/host-acls", options->home);
      s = lsh_get_cstring(tmp);
      fd = open(s, O_RDONLY);
      
      if (fd < 0)
	{
	  struct stat sbuf;
	  struct lsh_string *known_hosts
	    = ssh_format("%lz/.lsh/known_hosts", options->home);

	  if (stat(lsh_get_cstring(known_hosts), &sbuf) == 0)
	    {
	      werror("You have an old known-hosts file `%S'.\n"
		     "To work with lsh-2.0, run the lsh-upgrade script,\n"
		     "which will convert that to a new host-acls file.\n",
		     tmp);
	    }
	  lsh_string_free(known_hosts);
	}
    }

  if (fd < 0)
    {
      werror("Failed to open `%z' for reading %e\n", s, errno);
      lsh_string_free(tmp);

      return context;
    }

  lsh_string_free(tmp);

  if (!sexp_conv)
    sexp_conv = PATH_SEXP_CONV;
  
  contents = lsh_popen_read(sexp_conv, args, fd, 5000);
  
  if (!contents)
  {
    werror("Failed to read host-acls file %e\n", errno);
    close(fd);
    return context;
  }

  close(fd);

  /* We could use transport syntax instead. That would have the
   * advantage that we can read and process one entry at a time. */
  if (!spki_iterator_first(&i, STRING_LD(contents)))
    werror("read_known_hosts: S-expression syntax error.\n");
    
  else
    while (i.type != SPKI_TYPE_END_OF_EXPR)
      {
	if (!spki_add_acl(context, &i))
	  {
	    werror("read_known_hosts: Invalid ACL.\n");
	    break;
	  }
      }
  lsh_string_free(contents);
  return context;
}

/* For now, supports only a single key */
static struct object_list *
read_user_keys(struct lsh_options *options)
{
  struct lsh_string *tmp = NULL;
  struct lsh_string *contents;
  const char *name = NULL;
  int fd;
  int algorithm_name;
  
  struct signer *s;
  struct verifier *v;
  struct lsh_string *spki_public;

  trace("read_user_keys\n");
  
  if (!options->with_publickey)
    return make_object_list(0, -1);

  if (options->identity)
    name = options->identity;
  else 
    {
      tmp = ssh_format("%lz/.lsh/identity", options->home);
      name = lsh_get_cstring(tmp);
    }

  fd = open(name, O_RDONLY);
  if (fd < 0)
    {
      verbose("Failed to open `%z' for reading %e\n", name, errno);
      lsh_string_free(tmp);

      return make_object_list(0, -1);
    }

  lsh_string_free(tmp);

  contents = io_read_file_raw(fd, 2000);

  if (!contents)
    {
      werror("Failed to read private key file %e\n", errno);
      close(fd);

      return make_object_list(0, -1);
    }

  close(fd);

  if (options->super.tty)
    {
      struct alist *mac = make_alist(0, -1);
      struct alist *crypto = make_alist(0, -1);

      alist_select_l(mac, options->algorithms->algorithms,
		     2, ATOM_HMAC_SHA1, ATOM_HMAC_MD5, -1);
      alist_select_l(crypto, options->algorithms->algorithms,
		     4, ATOM_3DES_CBC, ATOM_BLOWFISH_CBC,
		     ATOM_TWOFISH_CBC, ATOM_AES256_CBC, -1);
      
      contents = spki_pkcs5_decrypt(mac, crypto,
				    options->super.tty,
				    contents);
      if (!contents)
        {
          werror("Decrypting private key failed.\n");
          return make_object_list(0, -1);
        }
    }
  
  s = spki_make_signer(options->signature_algorithms, contents,
		       &algorithm_name);

  lsh_string_free(contents);
  
  if (!s)
    {
      werror("Invalid private key.\n");
      return make_object_list(0, -1);
    }
  
  v = SIGNER_GET_VERIFIER(s);
  assert(v);
  
  spki_public = PUBLIC_SPKI_KEY(v, 0);

  /* Test key here? */  
  switch (algorithm_name)
    {	  
    case ATOM_DSA:
      return make_object_list(2,
                              make_keypair(ATOM_SSH_DSS,
                                           PUBLIC_KEY(v),
                                           s),
                              make_keypair(ATOM_SPKI_SIGN_DSS,
                                           spki_public, s),
                              -1);
      break;

    case ATOM_RSA_PKCS1:
    case ATOM_RSA_PKCS1_SHA1:
      return make_object_list(2, 
                              make_keypair(ATOM_SSH_RSA,
                                           PUBLIC_KEY(v),
                                           s),
                              make_keypair(ATOM_SPKI_SIGN_RSA,
                                           spki_public, s),
                              -1);

    case ATOM_RSA_PKCS1_MD5:
      return make_object_list(1,
                              make_keypair(ATOM_SPKI_SIGN_RSA,
                                           spki_public, s),
                              -1);

    default:
      fatal("Internal error!\n");
    }
}


/* Maps a host key to a (trusted) verifier object. */

/* GABA:
   (class
     (name lsh_host_db)
     (super lookup_verifier)
     (vars
       (db object spki_context)
       (tty object interact)
       (access string)
       (host . "const char *")
       ; Allow unauthorized keys
       (sloppy . int)
       ; If non-null, append an ACL for the received key to this file.
       (file object abstract_write)
       ; For fingerprinting
       (hash const object hash_algorithm)))
*/

static struct verifier *
do_lsh_lookup(struct lookup_verifier *c,
	      int method,
	      struct lsh_user *keyholder UNUSED,
	      struct lsh_string *key)
{
  CAST(lsh_host_db, self, c);
  struct spki_principal *subject;

  switch (method)
    {
    case ATOM_SSH_DSS:
      {	
	struct lsh_string *spki_key;
	struct verifier *v = make_ssh_dss_verifier(key);

	if (!v)
	  {
	    werror("do_lsh_lookup: Invalid ssh-dss key.\n");
	    return NULL;
	  }

	/* FIXME: It seems like a waste to pick apart the sexp again */
	spki_key = PUBLIC_SPKI_KEY(v, 0);

	subject = spki_lookup(self->db, STRING_LD(spki_key), v);
	assert(subject);
	assert(subject->verifier);

	lsh_string_free(spki_key);
	break;
      }
    case ATOM_SSH_RSA:
      {
	struct lsh_string *spki_key;
	struct verifier *v = make_ssh_rsa_verifier(key);

	if (!v)
	  {
	    werror("do_lsh_lookup: Invalid ssh-rsa key.\n");
	    return NULL;
	  }

	/* FIXME: It seems like a waste to pick apart the sexp again */
	spki_key = PUBLIC_SPKI_KEY(v, 0);
	subject = spki_lookup(self->db, STRING_LD(spki_key), v);
	assert(subject);
	assert(subject->verifier);

	lsh_string_free(spki_key);
	break;
      }
      
      /* It doesn't matter here which flavour of SPKI is used. */
    case ATOM_SPKI_SIGN_RSA:
    case ATOM_SPKI_SIGN_DSS:
      {
	subject = spki_lookup(self->db, STRING_LD(key), NULL);
	if (!subject)
	  {
	    werror("do_lsh_lookup: Invalid spki key.\n");
	    return NULL;
	  }
	if (!subject->verifier)
	  {
	    werror("do_lsh_lookup: Valid SPKI subject, but no key available.\n");
	    return NULL;
	  }
	break;
      }
    default:
      werror("do_lsh_lookup: Unknown key type. Should not happen!\n");
      return NULL;
    }

  assert(subject->key);
  
  /* Check authorization */

  if (spki_authorize(self->db, subject, time(NULL), self->access))
    {
      verbose("SPKI host authorization successful!\n");
    }
  else
    {
      struct lsh_string *acl;
      struct spki_iterator i;
      
      verbose("SPKI authorization failed.\n");
      if (!self->sloppy)
	{
	  werror("Server's hostkey is not trusted. Disconnecting.\n");
	  return NULL;
	}
      
      /* Ok, let's see if we want to use this untrusted key. */
      if (!quiet_flag)
	{
	  /* Display fingerprint */
	  /* FIXME: Rewrite to use libspki subject */
#if 0
	  struct lsh_string *spki_fingerprint = 
	    hash_string(self->hash, subject->key, 0);
#endif
	  
	  struct lsh_string *fingerprint = 
	    lsh_string_colonize( 
				ssh_format( "%lfxS", 
					    hash_string(&crypto_md5_algorithm,
							key,
							0)
					    ), 
				2, 
				1  
				);

	  struct lsh_string *babble = 
	    lsh_string_bubblebabble( 
				    hash_string(&crypto_sha1_algorithm,
						key,
						0),
				    1 
				    );
	  
	  if (!INTERACT_YES_OR_NO
	      (self->tty,
	       ssh_format("Received unauthenticated key for host %lz\n"
			  "Key details:\n"
			  "Bubble Babble: %lfS\n"
			  "Fingerprint:   %lfS\n"
			  /* "SPKI SHA1:     %lfxS\n" */
			  "Do you trust this key? (y/n) ",
			  self->host, babble, fingerprint /* , spki_fingerprint */), 0, 1))
	    return NULL;
	}

      acl = lsh_string_format_sexp(0, "(acl(entry(subject%l)%l))",
				   subject->key_length, subject->key,
				   STRING_LD(self->access));
      
      /* FIXME: Seems awkward to pick the acl apart again. */
      if (!spki_iterator_first(&i, STRING_LD(acl)))
	fatal("Internal error.\n");
      
      /* Remember this key. We don't want to ask again for key re-exchange */
      spki_add_acl(self->db, &i);

      /* Write an ACL to disk. */
      if (self->file)
	{
	  A_WRITE(self->file,
		  ssh_format("\n; ACL for host %lz\n"
			     "%lfS\n",
			     self->host, lsh_string_format_sexp(1, "%l", STRING_LD(acl))));

	  lsh_string_free(acl);
	}
    }
  
  return subject->verifier;
}

static struct lookup_verifier *
make_lsh_host_db(struct spki_context *db,
		 struct interact *tty,
		 const char *host,
		 int sloppy,
		 struct abstract_write *file)
{
  NEW(lsh_host_db, res);

  res->super.lookup = do_lsh_lookup;
  res->db = db;
  res->tty = tty;
  res->access = make_ssh_hostkey_tag(host);
  res->host = host;
  res->sloppy = sloppy;
  res->file = file;
  res->hash = &crypto_sha1_algorithm;

  return &res->super;
}

static struct command *
make_lsh_login(struct lsh_options *options,
	       struct object_list *keys)
{
  struct client_userauth_method *password
    = make_client_password_auth(options->super.tty);

  struct client_userauth_method *kbdinteract
    = make_client_kbdinteract_auth(options->super.tty);
  
  /* FIXME: Perhaps we should try "none" only when using SRP. */
  struct client_userauth_method *none
    = make_client_none_auth();

  struct object_list *methods;
  if (LIST_LENGTH(keys))
    methods = make_object_list(4,
			       none,
			       make_client_publickey_auth(keys),
			       password,
			       kbdinteract,
			       -1);
  else
    methods = make_object_list(3, none, password, kbdinteract, -1);
  
  return make_client_userauth
    (ssh_format("%lz", options->super.user),
     ATOM_SSH_CONNECTION, methods);
}

/* GABA:
   (expr
     (name make_lsh_connect)
     (params
       (resource object resource)
       (handshake object handshake_info)
       (db object lookup_verifier)
       (actions object object_list)
       (login object command))
     (expr (lambda (remote)
               ; What to do with the service
	       ((progn actions)
		 (init_connection_service
		   (login
		     (request_userauth_service
		       ; Start the ssh transport protocol
		       (protect resource connection_handshake
			 handshake
			 db
			 ; Connect using tcp
			 (connect_list remote)))))))))
*/


/* Option parsing */

const char *argp_program_version
= "lsh-" VERSION ", secsh protocol version " CLIENT_PROTOCOL_VERSION;

const char *argp_program_bug_address = BUG_ADDRESS;

#define ARG_NOT 0x400

#define OPT_PUBLICKEY 0x201

#define OPT_SLOPPY 0x202
#define OPT_STRICT 0x203
#define OPT_CAPTURE 0x204

#define OPT_HOST_DB 0x205

#define OPT_DH 0x206
#define OPT_SRP 0x207

#define OPT_STDIN 0x210
#define OPT_STDOUT 0x211
#define OPT_STDERR 0x212

#define OPT_FORK_STDIO 0x213

static const struct argp_option
main_options[] =
{
  /* Name, key, arg-name, flags, doc, group */
  { "identity", 'i',  "Identity key", 0, "Use this key to authenticate.", 0 },
  { "publickey", OPT_PUBLICKEY, NULL, 0,
    "Try publickey user authentication (default).", 0 },
  { "no-publickey", OPT_PUBLICKEY | ARG_NOT, NULL, 0,
    "Don't try publickey user authentication.", 0 },
  { "host-db", OPT_HOST_DB, "Filename", 0, "By default, ~/.lsh/host-acls", 0},
  { "sloppy-host-authentication", OPT_SLOPPY, NULL, 0,
    "Allow untrusted hostkeys.", 0 },
  { "strict-host-authentication", OPT_STRICT, NULL, 0,
    "Never, never, ever trust an unknown hostkey. (default)", 0 },
  { "capture-to", OPT_CAPTURE, "File", 0,
    "When a new hostkey is received, append an ACL expressing trust in the key. "
    "In sloppy mode, the default is ~/.lsh/captured_keys.", 0 },
#if WITH_SRP
  { "srp-keyexchange", OPT_SRP, NULL, 0, "Enable experimental SRP support.", 0 },
  { "no-srp-keyexchange", OPT_SRP | ARG_NOT, NULL, 0, "Disable experimental SRP support (default).", 0 },
#endif /* WITH_SRP */

  { "dh-keyexchange", OPT_DH, NULL, 0,
    "Enable DH support (default, unless SRP is being used).", 0 },

  { "no-dh-keyexchange", OPT_DH | ARG_NOT, NULL, 0, "Disable DH support.", 0 },
  
  /* Actions */
  { "forward-remote-port", 'R', "remote-port:target-host:target-port",
    0, "", CLIENT_ARGP_ACTION_GROUP },
  { "gateway", 'G', NULL, 0, "Setup a local gateway", 0 },

#if WITH_X11_FORWARD
  /* FIXME: Perhaps this should be moved from lsh.c to client.c? It
   * doesn't work with lshg. Or perhaps that can be fixed?
   * About the same problem applies to -R. */
  
  { "x11-forward", 'x', NULL, 0, "Enable X11 forwarding.", CLIENT_ARGP_MODIFIER_GROUP },
  { "no-x11-forward", 'x' | ARG_NOT, NULL, 0,
    "Disable X11 forwarding (default).", 0 },
#endif
  
  { NULL, 0, NULL, 0, NULL, 0 }
};


static const struct argp_child
main_argp_children[] =
{
  { &client_argp, 0, "", 0 },
  { &algorithms_argp, 0, "", 0 },
  { &werror_argp, 0, "", 0 },
  { NULL, 0, NULL, 0}
};

#define CASE_ARG(opt, attr, none)		\
  case opt:					\
    if (self->super.not)			\
      {						\
        self->super.not = 0;			\
						\
      case opt | ARG_NOT:			\
        self->attr = none;			\
        break;					\
      }						\
      						\
    self->attr = arg;				\
    break

#define CASE_FLAG(opt, flag)			\
  case opt:					\
    if (self->super.not)			\
      {						\
        self->super.not = 0;			\
						\
      case opt | ARG_NOT:			\
        self->flag = 0;				\
        break;					\
      }						\
      						\
    self->flag = 1;				\
    break

static error_t
main_argp_parser(int key, char *arg, struct argp_state *state)
{
  CAST(lsh_options, self, state->input);
  
  switch(key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    case ARGP_KEY_INIT:
      state->child_inputs[0] = &self->super;
      state->child_inputs[1] = self->algorithms;
      state->child_inputs[2] = NULL;
      break;
      
    case ARGP_KEY_END:
      if (self->super.inhibit_actions)
	break;

      if (!self->home)
	{
	  argp_error(state, "No home directory. Please set HOME in the environment.");
	  break;
	}

      if (!self->super.random)
	argp_failure( state, EXIT_FAILURE, 0,  "No randomness generator available.");

      if (self->with_dh_keyexchange < 0)
	self->with_dh_keyexchange = !self->with_srp_keyexchange;
      
      if (self->with_dh_keyexchange || self->with_srp_keyexchange)
	{
	  int i = 0;
	  self->kex_algorithms 
	    = alloc_int_list(2 * self->with_dh_keyexchange + self->with_srp_keyexchange);
	    
#if WITH_SRP	    
	  if (self->with_srp_keyexchange)
	    {
	      LIST(self->kex_algorithms)[i++] = ATOM_SRP_RING1_SHA1_LOCAL;
	      ALIST_SET(self->algorithms->algorithms,
			ATOM_SRP_RING1_SHA1_LOCAL,
			&make_srp_client(make_srp1(self->super.random),
					 self->super.tty,
					 ssh_format("%lz", self->super.user))
			->super);
	    }
#endif /* WITH_SRP */
	  if (self->with_dh_keyexchange)
	    {
	      LIST(self->kex_algorithms)[i++] = ATOM_DIFFIE_HELLMAN_GROUP14_SHA1;
	      ALIST_SET(self->algorithms->algorithms,
			ATOM_DIFFIE_HELLMAN_GROUP14_SHA1,
			&make_dh_client(make_dh14(self->super.random))
			->super);
	      
	      LIST(self->kex_algorithms)[i++] = ATOM_DIFFIE_HELLMAN_GROUP1_SHA1;
	      ALIST_SET(self->algorithms->algorithms,
			ATOM_DIFFIE_HELLMAN_GROUP1_SHA1,
			&make_dh_client(make_dh1(self->super.random))
			->super);
	    }
	}
      else
	argp_error(state, "All keyexchange algorithms disabled.");
	
      {
	struct lsh_string *tmp = NULL;
	const char *s = NULL;
	  
	if (self->capture)
	  s = self->capture;
	else if (self->sloppy)
	  {
	    tmp = ssh_format("%lz/.lsh/captured_keys", self->home);
	    s = lsh_get_cstring(tmp);
	  }
	if (s)
	  {
	    static const struct report_exception_info report =
	      STATIC_REPORT_EXCEPTION_INFO(EXC_IO, EXC_IO,
					   "Writing new ACL: ");

	    self->capture_file
	      = io_write_file(s,
			      O_CREAT | O_APPEND | O_WRONLY,
			      0600, 
			      make_report_exception_handler
			      (&report,
			       &default_exception_handler,
			       HANDLER_CONTEXT));
	    if (!self->capture_file)
	      werror("Failed to open '%z' %e.\n", s, errno);
	  }
	lsh_string_free(tmp);
      }

      /* We can't add the gateway action immediately when the -G
       * option is encountered, as we need the name and port of the
       * remote machine (self->super.remote) first.
       *
       * This breaks the rule that actions should be performed in the
       * order they are given on the command line. Since we usually
       * want the gateway action first (e.g. when the testsuite runs
       * lsh -G -B, and expects the gateway to be up by the time lsh
       * goes into the background), we prepend it on the list of
       * actions. */
      if (self->start_gateway)
	{
	  struct local_info *gateway;
	  if (!self->super.local_user)
	    {
	      argp_error(state, "You have to set LOGNAME in the environment, "
			 " if you want to use the gateway feature.");
	      break;
	    }
	  gateway = make_gateway_address(self->super.local_user,
					 self->super.user,
					 self->super.target);

	  if (!gateway)
	    {
	      argp_error(state, "Local or remote user name, or the target host name, are too "
			 "strange for the gateway socket name construction.");
	      break;
	    }
	      
	  client_prepend_action(&self->super,
				make_gateway_setup(gateway));
	}

      if (object_queue_is_empty(&self->super.actions))
	{
	  argp_error(state, "No actions given.");
	  break;
	}

      break;
      
    case 'i':
      self->identity = arg;
      break;

    CASE_FLAG(OPT_PUBLICKEY, with_publickey);

    case OPT_HOST_DB:
      self->known_hosts = arg;
      break;
      
    case OPT_SLOPPY:
      self->sloppy = 1;
      break;

    case OPT_STRICT:
      self->sloppy = 0;
      break;

    case OPT_CAPTURE:
      self->capture = arg;
      break;

    CASE_FLAG(OPT_DH, with_dh_keyexchange);
    CASE_FLAG(OPT_SRP, with_srp_keyexchange);

    case 'R':
      {
	uint32_t listen_port;
	struct address_info *target;

	if (!client_parse_forward_arg(arg, &listen_port, &target))
	  argp_error(state, "Invalid forward specification '%s'.", arg);

	client_add_action(&self->super, make_forward_remote_port
			  (make_address_info((self->super.with_remote_peers
					      ? ssh_format("%lz", "0.0.0.0")
					      : ssh_format("%lz", "127.0.0.1")),
					     listen_port),
			   target));

	self->super.remote_forward = 1;
	break;
      }      
      
    CASE_FLAG('G', start_gateway);
#if WITH_X11_FORWARD
    CASE_FLAG('x', super.with_x11);
#endif
    }

  return 0;
}

static const struct argp
main_argp =
{ main_options, main_argp_parser, 
  ( "host\n"
    "host command ..."), 
  ( "Connects to a remote machine\v"
    "Connects to the remote machine, and then performs one or more actions, "
    "i.e. command execution, various forwarding services. The default "
    "action is to start a remote interactive shell or execute a given "
    "command on the remote machine." ),
  main_argp_children,
  NULL, NULL
};

/* GABA:
   (class
     (name lsh_default_handler)
     (super exception_handler)
     (vars
       (status . "int *")))
*/

static void
do_lsh_default_handler(struct exception_handler *s,
		       const struct exception *e)
{
  CAST(lsh_default_handler, self, s);

  if (e->type & EXC_IO)
    {
      CAST_SUBTYPE(io_exception, exc, e);
      *self->status = EXIT_FAILURE;
      
      werror("%z, (errno = %i)\n", e->msg, exc->error);
      if (e->type == EXC_IO_CONNECT)
	exit(*self->status);
    }
  else
    switch(e->type)
      {
      case EXC_RESOLVE:
      case EXC_USERAUTH:
      case EXC_GLOBAL_REQUEST:
      case EXC_CHANNEL_REQUEST:
      case EXC_CHANNEL_OPEN:

	werror("%z\n", e->msg);
	*self->status = EXIT_FAILURE;
	break;
      default:
	*self->status = EXIT_FAILURE;
	EXCEPTION_RAISE(self->super.parent, e);
      }
}

static struct exception_handler *
make_lsh_default_handler(int *status, struct exception_handler *parent,
			 const char *context)
{
  NEW(lsh_default_handler, self);
  self->super.parent = parent;
  self->super.raise = do_lsh_default_handler;
  self->super.context = context;

  self->status = status;

  return &self->super;
}


int main(int argc, char **argv, const char** envp)
{
  struct lsh_options *options;
  struct spki_context *spki;
  struct object_list *keys;
  struct connect_list_state *remote;
  struct command *lsh_connect;
  
  /* Default exit code if something goes wrong. */
  int lsh_exit_code = 17;

  struct exception_handler *handler
    = make_lsh_default_handler(&lsh_exit_code, &default_exception_handler,
			       HANDLER_CONTEXT);

  io_init();
  
  /* For filtering messages. Could perhaps also be used when converting
   * strings to and from UTF8. */
  setlocale(LC_CTYPE, "");

  /* FIXME: Choose character set depending on the locale */
  set_local_charset(CHARSET_LATIN1);

  options = make_options(handler, &lsh_exit_code);

  if (!options)
    return EXIT_FAILURE;

  envp_parse(&main_argp, envp, "LSHFLAGS=", ARGP_IN_ORDER, options);
  argp_parse(&main_argp, argc, argv, ARGP_IN_ORDER, NULL, options);

  if (!options->super.random)
    {
      werror("Failed to initialize randomness generator.\n");
      return EXIT_FAILURE;
    }

  remote = make_connect_list_state();;
  
  if (!io_resolv_address(options->super.target,
			 options->super.port, 22,
			 &remote->q))
    {
      werror("Could not resolv address `%z'\n", options->super.target);
      return EXIT_FAILURE;
    }
  
  spki = read_known_hosts(options);
  keys = read_user_keys(options);
  
  lsh_connect =
    make_lsh_connect(
      &options->super.resources->super,
      make_handshake_info(CONNECTION_CLIENT,
			  SOFTWARE_SLOGAN, NULL,
			  SSH_MAX_PACKET,
			  options->super.random,
			  options->algorithms->algorithms,
			  make_simple_kexinit(
			    options->super.random,
			    options->kex_algorithms,
			    options->algorithms->hostkey_algorithms,
			    options->algorithms->crypto_algorithms,
			    options->algorithms->mac_algorithms,
			    options->algorithms->compression_algorithms,
			    make_int_list(0, -1)),
			  NULL),
      make_lsh_host_db(spki,
		       options->super.tty,
		       options->super.target,
		       options->sloppy,
		       options->capture_file),
      queue_to_list(&options->super.actions),
      make_lsh_login(options, keys));

  COMMAND_CALL(lsh_connect, remote, &discard_continuation,
	       handler);
  
  io_run();

  /* Close all files and other resources associated with the backend. */
  io_final();
  
  /* FIXME: Perhaps we have to reset the stdio file descriptors to
   * blocking mode? */
  return lsh_exit_code;
}
