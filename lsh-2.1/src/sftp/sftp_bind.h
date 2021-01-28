/*
 * @(#) $Id: b758b8e0509c2fd79663aabe23ea149fd44e8fd4 $
 *
 * sftp_bind.h 
 *
 * Portions of code taken from the sftp test client from
 * the sftp server of lsh by Niels Möller and myself.
 *
 */

/* lsftp, an implementation of the sftp protocol
 *
 * Copyright (C) 2001 Pontus Sköld
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

#ifndef LSFTP_SFTP_BIND_H
#define LSFTP_SFTP_BIND_H

#define LSH_CLIENT "lsh"
#define LSH_GATEWAY "lshg"
#define LSH_PROGENV "LSFTP_RSH"
#define BEFORE_ARGS_ENV "LSFTP_BEFORE_ARGS"
#define AFTER_ARGS_ENV "LSFTP_AFTER_ARGS"


#ifndef TRANSPORT_BEFORE_OPTS
#define DEFAULT_BEFORE_ARGS "--subsystem=sftp"
#else
#define DEFAULT_BEFORE_ARGS TRANSPORT_BEFORE_OPTS
#endif /* TRANSPORT_BEFORE_OPTS */


#ifndef TRANSPORT_AFTER_OPTS
#define DEFAULT_AFTER_ARGS ""
#else
#define DEFAULT_AFTER_ARGS TRANSPORT_AFTER_OPTS
#endif /* TRANSPORT_AFTER_OPTS */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <string.h>

#include "misc_fun.h"
#include "sftp_c.h"
#include "buffer.h"
#include "str_utils.h"
#include "dc.h"

#define KILL_WAIT 5

struct lsftp_callback;

typedef int (*lsftp_callback_func)(struct sftp_callback *s,
				   const struct lsftp_callback *l);

struct lsftp_callback
{
  int op_id;
  lsftp_callback_func nextfun;
  struct sftp_attrib *a;
  int free_a;

  const char *local;
  const char *remote;
  const char *command;
  void* memory;

  int opt1;
  int opt2;
  int opt3;
};

int lsftp_open_connection(char** argv, int argc); /* Open a new outgoing connection */
int lsftp_close_connection(void);                     /* Close an existing connection */

int lsftp_want_to_write(void);                        /* Returns whatever we want to write or not */

int lsftp_connected(void);    /* Are we connected? */

int lsftp_handshake(void);    /* Handshake with remote */
int lsftp_sftp_init(void);    /* Init sftp things */

int lsftp_callback(void);

int lsftp_cb_list(void);
int lsftp_cb_status( int jobid );


char* lsftp_pwd(void);

int lsftp_do_ls(const char* dir, const char* command, int longlist, int all);
int lsftp_handle_ls(struct sftp_callback *s,
		    const struct lsftp_callback *l);

int lsftp_internal_ls(const char *dir, const char *command,
		      const char*** dirinfop );
int lsftp_handle_internal_ls(struct sftp_callback *s,
			     const struct lsftp_callback *l);

int lsftp_do_get(const char *local, const char *remote,
		 const char *command, int cont); 
int lsftp_handle_get(struct sftp_callback *s,
		     const struct lsftp_callback *l);

int lsftp_do_put(const char *local, const char *remote,
		 const char *command, int cont);
int lsftp_handle_put(struct sftp_callback *s,
		     const struct lsftp_callback *l);

int lsftp_do_cd(const char *dir);

int lsftp_do_chmod(const char *file, mode_t mode, const char *command);
int lsftp_do_chown(const char *file, uint32_t uid, uint32_t gid,
		   const char *command);
int lsftp_handle_chall(struct sftp_callback *s,
		       const struct lsftp_callback *l);

int lsftp_do_stat(const char* file, struct sftp_attrib *a);
int lsftp_handle_stat(struct sftp_callback *s,
		      const struct lsftp_callback *l);

int lsftp_do_realpath(const char* file, char** destptr );
int lsftp_handle_realpath(struct sftp_callback* s,
			  const struct lsftp_callback *l);

int lsftp_do_mv(const char *src, const char *dst, const char *command);
int lsftp_do_rm(const char *path, const char *command);

int lsftp_do_ln(const char *link, const char *target, const char *command);

int lsftp_do_mkdir(const char *dir, int permissions, const char *command);
int lsftp_do_rmdir(const char *dir, const char *command);
int lsftp_handle_alldir(struct sftp_callback *s,
			const struct lsftp_callback *l);


struct lsftp_callback* lsftp_install_lsftp_cb( lsftp_callback_func nextfun );
int lsftp_install_sftp_cb(struct sftp_callback *s);

void lsftp_nullcb(  struct lsftp_callback* nullcb );

int lsftp_sftp_cb_init( int new_sftp_callbacks );
void lsftp_sftp_cb_uninit(void);
int lsftp_compact_sftp_cbs(void);


int lsftp_lsftp_cb_init( int new_lsftp_callbacks );
void lsftp_lsftp_cb_uninit(void);
int lsftp_compact_lsftp_cbs(void);

int lsftp_await_command( int id );

int lsftp_path_is_absolute( const char* path );
char *lsftp_qualify_path( const char *path );
const char *lsftp_unqualify_path( const char *path );

int lsftp_active_cbs(void);

void lsftp_perror(const char* msg, int err);

void lsftp_report_error(const struct sftp_callback *s,
			const struct lsftp_callback *l);

int lsftp_fd_read_net(void);
int lsftp_fd_write_net(void);

extern const char* status_codes_text[];

#endif /* LSFTP_SFTP_BIND_H */


