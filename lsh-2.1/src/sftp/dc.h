/*
 * @(#) $Id: 4a24e88075c355f21b81b8572c9b201b115abec1 $
 *
 * dc.h - dircache functions
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

#ifndef LSFTP_DC_H
#define LSFTP_DC_H

#include <stdio.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <glob.h>

#ifdef HAVE_FNMATCH
/* NOTE: Some version of Apache installs a file fnmatch.h, which
   includes ap_config.h, which contains a declaration of gethostname
   which is incompatible with unistd.h. We don't even try working
   around that. */
# include <fnmatch.h>
#endif

#include "sftp_bind.h"


int lsftp_dc_notice( char* name, struct sftp_attrib* a );
int lsftp_dc_remove( char* name );

int lsftp_dc_hash(const char* name );


int lsftp_dc_glob_matches(const char *fname, const char *glob, int period);

int lsftp_dc_l_isdir(const char* name);
int lsftp_dc_r_isdir(const char* name);

int lsftp_dc_index(const char *name);
int lsftp_dc_init( int new_dc_entries );
void lsftp_dc_uninit(void);

const char** lsftp_dc_r_startglob(const char *fname, int sloppy, int nocheck);
const char** lsftp_dc_r_contglob(const char *fname,
				 const char** globdata, int nocheck);

const char** lsftp_dc_l_startglob(const char *fname, int nocheck);
const char** lsftp_dc_l_contglob(const char *fname,
				 const char** globdata, int nocheck);
void lsftp_dc_endglob(const char** globdata);
int lsftp_dc_numglob(const char** globdata);

struct lsftp_dc_s { 
  char* name;
  int hash;
  /* Set if we know the permissions */
  int has_permissions;
  mode_t permissions;
};


#endif
