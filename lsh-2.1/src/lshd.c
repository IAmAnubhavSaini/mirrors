/* lshd.c
 *
 * Main server program.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004 Niels MÅˆller
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
/* #include <string.h> */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if TIME_WITH_SYS_TIME && HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "algorithms.h"
#include "alist.h"
#include "atoms.h"
#include "channel.h"
#include "channel_commands.h"
#include "charset.h"
#include "compress.h"
#include "connection_commands.h"
#include "crypto.h"
#include "daemon.h"
#include "environ.h"
#include "format.h"
#include "handshake.h"
#include "io.h"
#include "io_commands.h"
#include "lookup_verifier.h"
#include "randomness.h"
#include "reaper.h"
#include "server.h"
#include "server_authorization.h"
#include "server_keyexchange.h"
#include "server_pty.h"
#include "server_session.h"
#include "spki.h"
#include "srp.h"
#include "ssh.h"
#include "tcpforward.h"
#include "server_userauth.h"
#include "version.h"
#include "werror.h"
#include "xalloc.h"

#include "lsh_argp.h"

/* Forward declarations */
static struct install_info install_session_handler;
#define INSTALL_SESSION_HANDLER (&install_session_handler.super.super.super)

#include "lshd.c.x"


/* Option parsing */

const char *argp_program_version
= "lshd-" VERSION ", secsh protocol version " SERVER_PROTOCOL_VERSION;

const char *argp_program_bug_address = BUG_ADDRESS;

#define OPT_NO 0x400
#define OPT_INTERFACE 0x201

#define OPT_TCPIP_FORWARD 0x202
#define OPT_NO_TCPIP_FORWARD (OPT_TCPIP_FORWARD | OPT_NO)
#define OPT_PTY 0x203
#define OPT_NO_PTY (OPT_PTY | OPT_NO)
#define OPT_SUBSYSTEMS 0x204
#define OPT_NO_SUBSYSTEMS (OPT_SUBSYSTEMS | OPT_NO)

#define OPT_DAEMONIC 0x205
#define OPT_NO_DAEMONIC (OPT_DAEMONIC | OPT_NO)
#define OPT_PIDFILE 0x206
#define OPT_NO_PIDFILE (OPT_PIDFILE | OPT_NO)
#define OPT_CORE 0x207
#define OPT_SYSLOG 0x208
#define OPT_NO_SYSLOG (OPT_SYSLOG | OPT_NO)
#define OPT_X11_FORWARD 0x209
#define OPT_NO_X11_FORWARD (OPT_X11_FORWARD |OPT_NO)

#define OPT_SRP 0x210
#define OPT_NO_SRP (OPT_SRP | OPT_NO)
#define OPT_DH 0x211
#define OPT_NO_DH (OPT_DH | OPT_NO)

#define OPT_PUBLICKEY 0x220
#define OPT_NO_PUBLICKEY (OPT_PUBLICKEY | OPT_NO)
#define OPT_PASSWORD 0x221
#define OPT_NO_PASSWORD (OPT_PASSWORD | OPT_NO)

#define OPT_ROOT_LOGIN 0x222
#define OPT_NO_ROOT_LOGIN (OPT_ROOT_LOGIN | OPT_NO)

#define OPT_KERBEROS_PASSWD 0x223
#define OPT_NO_KERBEROS_PASSWD (OPT_KERBEROS_PASSWD | OPT_NO)

#define OPT_PASSWORD_HELPER 0x224

#define OPT_LOGIN_SHELL 0x225

#define OPT_TCPWRAPPERS 0x226
#define OPT_NO_TCPWRAPPERS 0x227

#define OPT_TCPWRAP_GOAWAY_MSG 0x228

#define OPT_LOGIN_AUTH_MODE 0x230
#define OPT_NO_LOGIN_AUTH_MODE (OPT_LOGIN_AUTH_MODE | OPT_NO)
#define OPT_LOGIN_AUTH_USER 0x231

#define OPT_BANNER_FILE 0x232

/* GABA:
   (class
     (name lshd_options)
     (super algorithms_options)
     (vars
       (reaper object reaper)
       (random object randomness)
       
       (signature_algorithms object alist)
       ;; Addresses to bind
       (local struct addr_queue)
       (port . "char *")
       (hostkey . "char *")
       (tcp_wrapper_name . "char *")
       (tcp_wrapper_message . "char *")

       (with_srp_keyexchange . int)
       (with_dh_keyexchange . int)

       ;; (kexinit object make_kexinit)
       (kex_algorithms object int_list)

       (with_loginauthmode . int)
       (with_publickey . int)
       (with_password . int)
       (allow_root . int)
       (pw_helper . "const char *")
       (login_shell . "const char *")
       ;; (loginauthmode_user . "const char *")

       (banner_file . "const char *")
       
       (with_tcpip_forward . int)
       (with_x11_forward . int)
       (with_pty . int)
       (subsystems . "const char **")
       
       (userauth_methods object int_list)
       (userauth_algorithms object alist)
       
       (daemonic . int)
       (no_syslog . int)
       (corefile . int)
       (pid_file . "const char *")
       ; -1 means use pid file iff we're in daemonic mode
       (use_pid_file . int)))
*/


static struct lshd_options *
make_lshd_options(void)
{
  NEW(lshd_options, self);

  init_algorithms_options(&self->super, all_symmetric_algorithms());

  self->reaper = make_reaper();
  self->random = make_system_random();

  /* OK to initialize with NULL */
  self->signature_algorithms = all_signature_algorithms(self->random);

  addr_queue_init(&self->local);
  
  /* Default behaviour is to lookup the "ssh" service, and fall back
   * to port 22 if that fails. */
  self->port = NULL;
  
  /* FIXME: this should perhaps use sysconfdir */  
  self->hostkey = "/etc/lsh_host_key";
  
  self->with_dh_keyexchange = 1;
  self->with_srp_keyexchange = 0;

  self->kex_algorithms = NULL;
  
  self->with_loginauthmode = 0;
  self->with_publickey = 1;
  self->with_password = 1;
  self->with_tcpip_forward = 1;
  /* Enabled by default. */
  self->with_x11_forward = 1;
  self->with_pty = 1;
  self->subsystems = NULL;

#if 0
  self->loginauthmode_user = NULL;
#endif
  self->banner_file = NULL;

  self->tcp_wrapper_name = "lshd";
  self->tcp_wrapper_message = NULL;

  self->allow_root = 0;
  self->pw_helper = NULL;
  self->login_shell = NULL;
  
  self->userauth_methods = NULL;
  self->userauth_algorithms = NULL;
  
  self->daemonic = 0;
  self->no_syslog = 0;
  
  /* FIXME: Make the default a configure time option? */
  self->pid_file = "/var/run/lshd.pid";
  self->use_pid_file = -1;
  self->corefile = 0;

  return self;
}

/* GABA:
   (class
     (name pid_file_resource)
     (super resource)
     (vars
       (file . "const char *")))
*/

static void
do_kill_pid_file(struct resource *s)
{
  CAST(pid_file_resource, self, s);
  if (self->super.alive)
    {
      self->super.alive = 0;
      if (unlink(self->file) < 0)
	werror("Unlinking pidfile failed %e\n", errno);
    }
}

static struct resource *
make_pid_file_resource(const char *file)
{
  NEW(pid_file_resource, self);
  init_resource(&self->super, do_kill_pid_file);
  self->file = file;

  return &self->super;
}

/* GABA:
   (class
     (name sighup_close_callback)
     (super lsh_callback)
     (vars
       (resource object resource)))
*/

static void
do_sighup_close_callback(struct lsh_callback *s)
{
  CAST(sighup_close_callback, self, s);
  unsigned nfiles;
  
  werror("SIGHUP received.\n");
  KILL_RESOURCE(self->resource);
  
  nfiles = io_nfiles();

  if (nfiles)
    werror("Waiting for active connections to terminate, "
	   "%i files still open.\n", nfiles);
}

static struct lsh_callback *
make_sighup_close_callback(struct resource *resource)
{
  NEW(sighup_close_callback, self);
  self->super.f = do_sighup_close_callback;
  self->resource = resource;

  return &self->super;
}

static const struct argp_option
main_options[] =
{
  /* Name, key, arg-name, flags, doc, group */
  { "interface", OPT_INTERFACE, "interface", 0,
    "Listen on this network interface.", 0 }, 
  { "port", 'p', "Port", 0, "Listen on this port.", 0 },
  { "host-key", 'h', "Key file", 0, "Location of the server's private key.", 0},
  { "banner-file", OPT_BANNER_FILE, "File name", 0, "Banner file to send before " "handshake.", 9 },

#if WITH_TCPWRAPPERS
  { NULL, 0, NULL, 0, "Connection filtering:", 0 },
  { "tcpwrappers", OPT_TCPWRAPPERS, "name", 0, "Set service name for tcp wrappers (default lshd)", 0 },
  { "no-tcpwrappers", OPT_NO_TCPWRAPPERS, NULL, 0, "Disable wrappers", 0 },
  { "tcpwrappers-msg", OPT_TCPWRAP_GOAWAY_MSG, "'Message'", 0, "Message sent to clients " 
    "who aren't allowed to connect. A newline will be added.", 0 },
#endif /* WITH_TCPWRAPPERS */

  { NULL, 0, NULL, 0, "Keyexchange options:", 0 },
#if WITH_SRP
  { "srp-keyexchange", OPT_SRP, NULL, 0, "Enable experimental SRP support.", 0 },
  { "no-srp-keyexchange", OPT_NO_SRP, NULL, 0, "Disable experimental SRP support (default).", 0 },
#endif /* WITH_SRP */

  { "dh-keyexchange", OPT_DH, NULL, 0, "Enable DH support (default).", 0 },
  { "no-dh-keyexchange", OPT_NO_DH, NULL, 0, "Disable DH support.", 0 },
  
  { NULL, 0, NULL, 0, "User authentication options:", 0 },

  { "login-auth-mode", OPT_LOGIN_AUTH_MODE, NULL, 0, 
    "Enable a telnet like mode (accept none-authentication and launch the" 
    "login-shell, making it responsible for authenticating the user).", 0 },

  { "no-login-auth-mode", OPT_NO_LOGIN_AUTH_MODE, NULL, 0, 
    "Disable login-auth-mode (default).", 0 },

#if 0
  { "login-auth-mode-user", OPT_LOGIN_AUTH_USER, "username", 0,
    "Run login-program as this user, defaults to the user "
    "who started lshd.", 0 },
#endif
  
  { "password", OPT_PASSWORD, NULL, 0,
    "Enable password user authentication (default).", 0},
  { "no-password", OPT_NO_PASSWORD, NULL, 0,
    "Disable password user authentication.", 0},

  { "publickey", OPT_PUBLICKEY, NULL, 0,
    "Enable publickey user authentication (default).", 0},
  { "no-publickey", OPT_NO_PUBLICKEY, NULL, 0,
    "Disable publickey user authentication.", 0},

  { "root-login", OPT_ROOT_LOGIN, NULL, 0,
    "Allow root to login.", 0 },
  { "no-root-login", OPT_NO_ROOT_LOGIN, NULL, 0,
    "Don't allow root to login (default).", 0 },

  { "login-shell", OPT_LOGIN_SHELL, "Program", 0,
    "Use this program as the login shell for all users. "
    "(Experimental)", 0 },
  
  { "kerberos-passwords", OPT_KERBEROS_PASSWD, NULL, 0,
    "Recognize kerberos passwords, using the helper program "
    "\"" PATH_KERBEROS_HELPER "\". This option is experimental.", 0 },
  { "no-kerberos-passwords", OPT_NO_KERBEROS_PASSWD, NULL, 0,
    "Don't recognize kerberos passwords (default behaviour).", 0 },

  { "password-helper", OPT_PASSWORD_HELPER, "Program", 0,
    "Use the named helper program for password verification. "
    "(Experimental).", 0 },

  { NULL, 0, NULL, 0, "Offered services:", 0 },

#if WITH_PTY_SUPPORT
  { "pty-support", OPT_PTY, NULL, 0, "Enable pty allocation (default).", 0 },
  { "no-pty-support", OPT_NO_PTY, NULL, 0, "Disable pty allocation.", 0 },
#endif /* WITH_PTY_SUPPORT */
#if WITH_TCP_FORWARD
  { "tcpip-forward", OPT_TCPIP_FORWARD, NULL, 0,
    "Enable tcpip forwarding (default).", 0 },
  { "no-tcpip-forward", OPT_NO_TCPIP_FORWARD, NULL, 0,
    "Disable tcpip forwarding.", 0 },
#endif /* WITH_TCP_FORWARD */
#if WITH_X11_FORWARD
  { "x11-forward", OPT_X11_FORWARD, NULL, 0,
    "Enable x11 forwarding (default).", 0 },
  { "no-x11-forward", OPT_NO_X11_FORWARD, NULL, 0,
    "Disable x11 forwarding.", 0 },
#endif /* WITH_X11_FORWARD */
  
  { "subsystems", OPT_SUBSYSTEMS, "List of subsystem names and programs", 0,
    "For example `sftp=/usr/sbin/sftp-server,foosystem=/usr/bin/foo' "
    "(experimental).", 0},
  
  { NULL, 0, NULL, 0, "Daemonic behaviour", 0 },
  { "daemonic", OPT_DAEMONIC, NULL, 0, "Run in the background, redirect stdio to /dev/null, and chdir to /.", 0 },
  { "no-daemonic", OPT_NO_DAEMONIC, NULL, 0, "Run in the foreground, with messages to stderr (default).", 0 },
  { "pid-file", OPT_PIDFILE, "file name", 0, "Create a pid file. When running in daemonic mode, "
    "the default is /var/run/lshd.pid.", 0 },
  { "no-pid-file", OPT_NO_PIDFILE, NULL, 0, "Don't use any pid file. Default in non-daemonic mode.", 0 },
  { "enable-core", OPT_CORE, NULL, 0, "Dump core on fatal errors (disabled by default).", 0 },
  { "no-syslog", OPT_NO_SYSLOG, NULL, 0, "Don't use syslog (by default, syslog is used "
    "when running in daemonic mode).", 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static const struct argp_child
main_argp_children[] =
{
  { &algorithms_argp, 0, "", 0 },
  { &werror_argp, 0, "", 0 },
  { NULL, 0, NULL, 0}
};

/* NOTE: Modifies the argument string. */
static const char **
parse_subsystem_list(char *arg)
{
  const char **subsystems;
  char *separator;
  unsigned length;
  unsigned i;
  
  /* First count the number of elements. */
  for (length = 1, i = 0; arg[i]; i++)
    if (arg[i] == ',')
      length++;

  subsystems = lsh_space_alloc((length * 2 + 1) * sizeof(*subsystems));

  for (i = 0; ; i++)
    {
      subsystems[2*i] = arg;

      separator = strchr(arg, '=');

      if (!separator)
	goto fail;

      *separator = '\0';

      subsystems[2*i+1] = arg = separator + 1;
      
      separator = strchr(arg, ',');

      if (i == (length - 1))
	break;
      
      if (!separator)
	goto fail;

      *separator = '\0';
      arg = separator + 1;
    }
  if (separator)
    {
    fail:
      lsh_space_free(subsystems);
      return NULL;
    }
  return subsystems;
}

/* NOTE: On success, modifies interface destructively. */
static int
parse_interface(char *interface, const char **host, const char **port)
{
  *port = NULL;
  
  if (interface[0] == '[')
    {
      /* A literal address */
      char *end;
      interface++;
      
      end = strchr(interface, ']');
      if (!end)
	return 0;

      switch (end[1])
	{
	case ':':
	  *port = end + 2;
	  break;
	case 0:
	  break;
	default:
	  return 0;
	}

      *host = interface;
      *end = 0;
      return 1;
    }
  else
    {
      char *end = strchr(interface, ':');
      if (end)
	{
	  *port = end + 1;
	  *end = 0;
	}
      *host = interface;
      return 1;
    }
}

static error_t
main_argp_parser(int key, char *arg, struct argp_state *state)
{
  CAST(lshd_options, self, state->input);
  
  switch(key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    case ARGP_KEY_INIT:
      state->child_inputs[0] = &self->super;
      state->child_inputs[1] = NULL;
      break;
    case ARGP_KEY_END:
      {
	struct user_db *user_db = NULL;
	
	if (!self->random)
	  argp_failure( state, EXIT_FAILURE, 0,  "No randomness generator available.");
	
       	if (self->with_password || self->with_publickey || self->with_srp_keyexchange)
	  user_db = make_unix_user_db(self->reaper,
				      self->pw_helper, self->login_shell,
				      self->allow_root);
	  
	if (self->with_dh_keyexchange || self->with_srp_keyexchange)
	  {
	    int i = 0;
	    self->kex_algorithms 
	      = alloc_int_list(2 * self->with_dh_keyexchange + self->with_srp_keyexchange);
	    
	    if (self->with_dh_keyexchange)
	      {
		LIST(self->kex_algorithms)[i++] = ATOM_DIFFIE_HELLMAN_GROUP14_SHA1;
		ALIST_SET(self->super.algorithms,
			  ATOM_DIFFIE_HELLMAN_GROUP14_SHA1,
			  &make_dh_server(make_dh14(self->random))
			  ->super);

		LIST(self->kex_algorithms)[i++] = ATOM_DIFFIE_HELLMAN_GROUP1_SHA1;
		ALIST_SET(self->super.algorithms,
			  ATOM_DIFFIE_HELLMAN_GROUP1_SHA1,
			  &make_dh_server(make_dh1(self->random))
			  ->super);
	      }
#if WITH_SRP	    
	    if (self->with_srp_keyexchange)
	      {
		assert(user_db);
		LIST(self->kex_algorithms)[i++] = ATOM_SRP_RING1_SHA1_LOCAL;
		ALIST_SET(self->super.algorithms,
			  ATOM_SRP_RING1_SHA1_LOCAL,
			  &make_srp_server(make_srp1(self->random),
					   user_db)
			  ->super);
	      }
#endif /* WITH_SRP */
	  }
	else
	  argp_error(state, "All keyexchange algorithms disabled.");

	if (addr_queue_is_empty(&self->local))
	  {
	    /* Default interface */

	    if (!(self->port
		  ? io_resolv_address(NULL, self->port, 0, &self->local)
		  : io_resolv_address(NULL, "ssh", 22, &self->local)))
		
	      argp_failure(state, EXIT_FAILURE, 0,
			   "Strange. Could not resolve the ANY address.");
	  }
	assert(!addr_queue_is_empty(&self->local));
	
	if (self->use_pid_file < 0)
	  self->use_pid_file = self->daemonic;

	self->userauth_algorithms = make_alist(0, -1);

	if (self->with_password || self->with_publickey)
	  {
	    int i = 0;
	    
	    self->userauth_methods
	      = alloc_int_list(self->with_password + self->with_publickey);
	    
	    if (self->with_password)
	      {
		LIST(self->userauth_methods)[i++] = ATOM_PASSWORD;
		ALIST_SET(self->userauth_algorithms,
			  ATOM_PASSWORD,
			  &make_userauth_password(user_db)->super);
	      }
	    if (self->with_publickey)
	      {
		/* FIXME: Doesn't use spki */
		struct lookup_verifier *key_db
		  = make_authorization_db(ssh_format("authorized_keys_sha1"),
					  &crypto_sha1_algorithm);
		
		LIST(self->userauth_methods)[i++] = ATOM_PUBLICKEY;
		ALIST_SET(self->userauth_algorithms,
			  ATOM_PUBLICKEY,
			  &make_userauth_publickey
			  (user_db,
			   make_alist(2,
				      ATOM_SSH_DSS, key_db,
				      ATOM_SSH_RSA, key_db,
				      -1))
			  ->super);
	      }
	  }


        if (self->with_srp_keyexchange)
          ALIST_SET(self->userauth_algorithms,
                    ATOM_NONE,
		    /* username must match the name from the key
		       exchange */		    
                    &server_userauth_none_preauth.super);
	else if (self->with_loginauthmode)
	  {
	    const char *name;

	    USER_NAME_FROM_ENV(name);
	    if (!name)
	      argp_failure(state, EXIT_FAILURE, 0,
			   "$LOGNAME not set in the environment.\n");

	    ALIST_SET(self->userauth_algorithms,
		      ATOM_NONE,
		      &make_userauth_none_permit
		      (make_unix_user_self(make_string(name),
					   self->reaper,
					   /* Make home dir configurable? */
					   "/",
					   self->login_shell))->super);
	  }

        if (!self->userauth_algorithms->size)
	  argp_error(state, "All user authentication methods disabled.");

	break;
      }
    case 'p':
      /* FIXME: Interpret multiple -p:s as a request to listen on
       * several ports. */
      self->port = arg;
      break;

    case 'h':
      self->hostkey = arg;
      break;

    case OPT_INTERFACE:
      {
	const char *host;
	const char *port;

	/* On success, modifies arg destructively. */
	if (!parse_interface(arg, &host, &port))
	  argp_error(state, "Invalid interface, port or service: %s.", arg);

	if (!port)
	  port = self->port;
	
	if (!(port
	      ? io_resolv_address(host, port, 0, &self->local)
	      : io_resolv_address(host, "ssh", 22, &self->local)))
	  argp_failure(state, EXIT_FAILURE, 0,
		       "Address %s:%s could not be resolved.\n",
		       host, port ? port : "ssh");
      }
	
      break;

    case OPT_SRP:
      self->with_srp_keyexchange = 1;
      break;

    case OPT_NO_SRP:
      self->with_srp_keyexchange = 0;
      break;
      
    case OPT_DH:
      self->with_dh_keyexchange = 1;
      break;

    case OPT_NO_DH:
      self->with_dh_keyexchange = 0;
      break;
      
    case OPT_PASSWORD:
      self->with_password = 1;
      break;
      
    case OPT_NO_PASSWORD:
      self->with_password = 0;
      break;

    case OPT_PUBLICKEY:
      self->with_publickey = 1;
      break;
      
    case OPT_NO_PUBLICKEY:
      self->with_publickey = 0;
      break;

    case OPT_LOGIN_AUTH_MODE:
      self->with_loginauthmode = 1;
      break;

    case OPT_NO_LOGIN_AUTH_MODE:
      self->with_loginauthmode = 0;
      break;

#if 0
    case OPT_LOGIN_AUTH_USER:
      self->loginauthmode_user = arg;
      break;
#endif
      
    case OPT_BANNER_FILE:
      self->banner_file = arg;
      break;

    case OPT_ROOT_LOGIN:
      self->allow_root = 1;
      break;

    case OPT_NO_ROOT_LOGIN:
      self->allow_root = 0;
      break;

    case OPT_KERBEROS_PASSWD:
      self->pw_helper = PATH_KERBEROS_HELPER;
      break;

    case OPT_NO_KERBEROS_PASSWD:
      self->pw_helper = NULL;
      break;

    case OPT_PASSWORD_HELPER:
      self->pw_helper = arg;
      break;

    case OPT_LOGIN_SHELL:
      self->login_shell = arg;
      break;
      
#if WITH_TCP_FORWARD
    case OPT_TCPIP_FORWARD:
      self->with_tcpip_forward = 1;
      break;

    case OPT_NO_TCPIP_FORWARD:
      self->with_tcpip_forward = 0;
      break;
#endif /* WITH_TCP_FORWARD */
#if WITH_X11_FORWARD
    case OPT_X11_FORWARD:
      self->with_x11_forward = 1;
      break;
    case OPT_NO_X11_FORWARD:
      self->with_x11_forward = 0;
      break;
#endif /* WITH_X11_FORWARD */
      
#if WITH_PTY_SUPPORT
    case OPT_PTY:
      self->with_pty = 1;
      break;
    case OPT_NO_PTY:
      self->with_pty = 0;
      break;
#endif /* WITH_PTY_SUPPORT */

#if WITH_TCPWRAPPERS
    case OPT_TCPWRAPPERS:
      self->tcp_wrapper_name = arg; /* Name given */
      break;
    case OPT_NO_TCPWRAPPERS:
      self->tcp_wrapper_name = NULL; /* Disable by giving name NULL */
      break;
      
    case OPT_TCPWRAP_GOAWAY_MSG:
      self->tcp_wrapper_message = arg;
      break;

#endif /* WITH_TCPWRAPPERS */

    case OPT_SUBSYSTEMS:
      self->subsystems = parse_subsystem_list(arg);
      if (!self->subsystems)
	argp_error(state, "Invalid subsystem list.");
      break;

    case OPT_NO_SUBSYSTEMS:
      self->subsystems = NULL;
      break;
      
    case OPT_DAEMONIC:
      self->daemonic = 1;
      break;
      
    case OPT_NO_DAEMONIC:
      self->daemonic = 0;
      break;

    case OPT_NO_SYSLOG:
      self->no_syslog = 1;
      break;
      
    case OPT_PIDFILE:
      self->pid_file = arg;
      self->use_pid_file = 1;
      break;

    case OPT_NO_PIDFILE:
      self->use_pid_file = 0;
      break;

    case OPT_CORE:
      self->corefile = 1;
      break;
    }
  return 0;
}

static const struct argp
main_argp =
{ main_options, main_argp_parser, 
  NULL,
  "Server for the ssh-2 protocol.",
  main_argp_children,
  NULL, NULL
};


static void
do_terminate_callback(struct lsh_callback *s UNUSED)
{
  io_final();

  /* If we're using GCOV, just call exit(). That way, profiling info
   * is written properly when the process is terminated. */
#if !WITH_GCOV
  kill(getpid(), SIGKILL);
#endif
  exit(0);
}

static struct lsh_callback
sigterm_handler = { STATIC_HEADER, do_terminate_callback };

static void
install_signal_handlers(struct resource *resource)
{
  io_signal_handler(SIGTERM, &sigterm_handler);
  io_signal_handler(SIGHUP,
		    make_sighup_close_callback(resource));
}

static void
do_exc_lshd_handler(struct exception_handler *self,
		    const struct exception *e)
{
  if (e->type & EXC_IO)
    {
      CAST_SUBTYPE(io_exception, exc, e);
      
      werror("%z, (errno = %i)\n", e->msg, exc->error);
    }
  else
    EXCEPTION_RAISE(self->parent, e);
}

static struct exception_handler lshd_exception_handler
= STATIC_EXCEPTION_HANDLER(do_exc_lshd_handler, &default_exception_handler);

/* Invoked when starting the ssh-connection service */
/* GABA:
   (expr
     (name lshd_connection_service)
     (params
       (hooks object object_list))
     (expr
       (lambda (connection)
         ((progn hooks)
	    ; We have to initialize the connection
	    ; before adding handlers.
	    (init_connection_service
	      ; Disconnect if connection->user is NULL
	      (connection_require_userauth connection)))))))
*/

static struct command *
make_lshd_connection_service(struct lshd_options *options)
{
  /* Commands to be invoked on the connection */
  /* FIXME: Use a queue instead. */
  struct object_list *connection_hooks;
  struct command *session_setup;
  struct alist *supported_channel_requests;

  /* Supported channel requests */


  supported_channel_requests = make_alist(2,
					  ATOM_SHELL, &shell_request_handler,
					  ATOM_EXEC, &exec_request_handler,
					  -1);
    
#if WITH_PTY_SUPPORT
  if (options->with_pty)
    {
      ALIST_SET(supported_channel_requests,
		ATOM_PTY_REQ, &pty_request_handler.super);
      ALIST_SET(supported_channel_requests,
		ATOM_WINDOW_CHANGE, &window_change_request_handler.super);
    }
#endif /* WITH_PTY_SUPPORT */

#if WITH_X11_FORWARD
  if (options->with_x11_forward)
    ALIST_SET(supported_channel_requests,
	      ATOM_X11_REQ, &x11_req_handler.super);
#endif /* WITH_X11_FORWARD */
  
  if (options->subsystems)
    ALIST_SET(supported_channel_requests,
	      ATOM_SUBSYSTEM,
	      &make_subsystem_handler(options->subsystems)->super);
  
  session_setup = make_install_fix_channel_open_handler
    (ATOM_SESSION, make_open_session(supported_channel_requests));
  
#if WITH_TCP_FORWARD
  if (options->with_tcpip_forward)
    connection_hooks = make_object_list
      (4,
       session_setup,
       make_tcpip_forward_hook(),
       make_install_fix_global_request_handler
       (ATOM_CANCEL_TCPIP_FORWARD, &tcpip_cancel_forward),
       make_direct_tcpip_hook(),
       -1);
  else
#endif
    connection_hooks
      = make_object_list (1, session_setup, -1); 

  {
    CAST_SUBTYPE(command, connection_service,
		 lshd_connection_service(connection_hooks));
    return connection_service;
  }
}

/* Command to install session handler */
static struct install_info install_session_handler =
STATIC_INSTALL_OPEN_HANDLER(ATOM_SESSION);

/* Invoked when starting the ssh-connection service */ 
/* GABA: 
   (expr 
     (name lshd_login_service) 
     (params
       (handler object channel_open))
     (expr 
       (lambda (connection) 
         (install_session_handler
            ; We have to initialize the connection 
	    ; before adding handlers.
	    (init_connection_service
	      ; The fix user object is installed by the
	      ; userauth "none" handler.
	      (connection_require_userauth connection))
	    handler))))
*/ 
  
static struct command *
make_lshd_login_service(struct lshd_options *options)
{
  struct alist *supported_channel_requests;
  
#if WITH_PTY_SUPPORT
  if (options->with_pty)
    {
      supported_channel_requests
	= make_alist(3,
		     ATOM_SHELL, &shell_request_handler,
		     ATOM_PTY_REQ, &pty_request_handler.super,
		     ATOM_WINDOW_CHANGE, &window_change_request_handler.super,
		     -1);
    }
  else
#endif /* WITH_PTY_SUPPORT */
    supported_channel_requests
      = make_alist(1,
		   ATOM_SHELL, &shell_request_handler,
		   -1);

  {
    CAST_SUBTYPE(command, connection_service,
		 lshd_login_service(make_open_session
				    (supported_channel_requests)));
    return connection_service;
  }
}


/* GABA:
   (expr
     (name lshd_listen_callback)
     (params
       (handshake object handshake_info)
       (keys object alist)
       (logger object command)
       (services object command))
     (expr (lambda (lv)
    	      (services (connection_handshake NULL
	                   handshake
			   keys
			   (logger lv))))))
*/

static struct io_callback *
make_lshd_listen_callback(struct lshd_options *options,
			  struct alist *keys,
			  struct command *service)
{
  struct int_list *hostkey_algorithms;
  struct make_kexinit *kexinit;
  struct command *logger;
  struct lsh_string *banner_text = NULL;
  
  /* Include only hostkey algorithms that we have keys for. */
  hostkey_algorithms
    = filter_algorithms(keys,
			options->super.hostkey_algorithms);
  
  if (!hostkey_algorithms)
    {
      werror("No hostkey algorithms advertised.\n");
      hostkey_algorithms = make_int_list(1, ATOM_NONE, -1);
    }
  
  kexinit = make_simple_kexinit(options->random,
				options->kex_algorithms,
				hostkey_algorithms,
				options->super.crypto_algorithms,
				options->super.mac_algorithms,
				options->super.compression_algorithms,
				make_int_list(0, -1));

#if WITH_TCPWRAPPERS
  if (options->tcp_wrapper_name)
    logger = make_tcp_wrapper
      (make_string(options->tcp_wrapper_name),
       make_string(options->tcp_wrapper_message
		   ? options->tcp_wrapper_message : ""));
  else
#endif /* WITH_TCPWRAPPERS */
    {
      logger = &io_log_peer_command;

    }

  if (options->banner_file) /* Banner? */
    {

      int fd = open(options->banner_file, 0);
      
      if (fd >= 0)
	{
	  banner_text = io_read_file_raw(fd, 1024);
	  close(fd);
	}
    }



  {
    CAST_SUBTYPE(command, server_callback,
		 lshd_listen_callback
		   (make_handshake_info(CONNECTION_SERVER,
					SOFTWARE_SLOGAN,
					NULL,
					SSH_MAX_PACKET,
					options->random,
					options->super.algorithms,
					kexinit,
				        banner_text),
		  keys,
		  logger,
		  make_offer_service
		  (make_alist
		   (1,
		    ATOM_SSH_USERAUTH,
		    make_userauth_service(options->userauth_methods,
					  options->userauth_algorithms,
					  make_alist(1, ATOM_SSH_CONNECTION,
						     service,-1)),
		    -1))));

    /* There should be no exceptions raised on the fd:s we listen on */
    return make_listen_callback(server_callback, &lshd_exception_handler);
  }
}

int
main(int argc, char **argv)
{
  struct lshd_options *options;
  enum daemon_mode mode = DAEMON_NORMAL;

  /* Resources that should be killed when SIGHUP is received,
   * or when the program exits. */
  struct resource_list *resources = make_resource_list();

  /* Hostkeys */
  struct alist *keys = make_alist(0, -1);

  struct resource *fds;

  /* Do this first and unconditionally, before we start to initialize i/o */
  daemon_close_fds();
  
#if HAVE_SETRLIMIT && HAVE_SYS_RESOURCE_H
  /* Try to increase max number of open files, ignore any error */

  struct rlimit r;

  r.rlim_max = RLIM_INFINITY;
  r.rlim_cur = RLIM_INFINITY;

  setrlimit(RLIMIT_NOFILE, &r);
#endif

  /* Not strictly needed for gc, but makes sure the
   * resource list is killed properly by gc_final. */
  gc_global(&resources->super);

  io_init();
  
  /* For filtering messages. Could perhaps also be used when converting
   * strings to and from UTF8. */
  setlocale(LC_CTYPE, "");

  /* FIXME: Choose character set depending on the locale */
  set_local_charset(CHARSET_LATIN1);

  install_signal_handlers(&resources->super);

  options = make_lshd_options();

  trace("Parsing options...\n");
  argp_parse(&main_argp, argc, argv, 0, NULL, options);
  trace("Parsing options... done\n");  

  if (options->daemonic)
    {
      mode = daemon_detect();

      if (mode == DAEMON_INETD)
	{
	  werror("Spawning from inetd not yet supported.\n");
	  return EXIT_FAILURE;
	}

      if (!options->no_syslog)
	{
#if HAVE_SYSLOG
	  set_error_syslog("lshd");
#else /* !HAVE_SYSLOG */
	  werror("lshd: No syslog. Further messages will be directed to /dev/null.\n");
#endif /* !HAVE_SYSLOG */
	  if (!daemon_dup_null(STDERR_FILENO))
	    return EXIT_FAILURE;
	}

      if (mode != DAEMON_INETD)
	{
	  if (!daemon_dup_null(STDIN_FILENO)
	      || !daemon_dup_null(STDOUT_FILENO))
	    return EXIT_FAILURE;
	}
    }

  if (!options->corefile && !daemon_disable_core())
    {
      werror("Disabling of core dumps failed.\n");
      return EXIT_FAILURE;
    }

  if (!options->random) 
    {
      werror("Failed to initialize randomness generator.\n");
      return EXIT_FAILURE;
    }

  if (!read_host_key(options->hostkey, options->signature_algorithms, keys))
    return EXIT_FAILURE;

  if (options->with_loginauthmode) 
    fds = io_listen_list(&options->local, 
			 make_lshd_listen_callback 
			 (options, keys, 
			  make_lshd_login_service(options)), 
			 &lshd_exception_handler);           
  else 
    fds = io_listen_list(&options->local, 
			 make_lshd_listen_callback 
			 (options, keys, 
			  make_lshd_connection_service(options)), 
			 &lshd_exception_handler); 
  
  if (!fds)
    {
      werror("Could not bind any address.\n");
      return EXIT_FAILURE;
    }

  remember_resource(resources, fds);
  
  if (options->daemonic)
    {
      if (!daemon_init(mode))
	{
	  werror("Setting up daemonic environment failed.\n");
	  return EXIT_FAILURE;
	}
    }
  
  if (options->use_pid_file)
    {
      if (daemon_pidfile(options->pid_file))
	remember_resource(resources, 
			  make_pid_file_resource(options->pid_file));
      else
	{
	  werror("lshd seems to be running already.\n");
	  return EXIT_FAILURE;
	}
    }
  
  io_run();

  io_final();
  
  return 0;
}
