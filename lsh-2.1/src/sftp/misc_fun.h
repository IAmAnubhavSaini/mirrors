/*
 * @(#) $Id: a966bfe26257bc6a6b53944d28d5e884633944b8 $
 *
 * misc_fun.h
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

#ifndef LSFTP_MISC_FUN_H
#define LSFTP_MISC_FUN_H

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

char* mgetenv( char* var );
void mgetenv_init( char** envp );

char* massage_fname( char* fname, int local );
const char *filename_part(const char *s);
char* lsftp_concat( const char* s1, const char* s2 );
const char *lsftp_skip_common(const char *s1, const char *s2);
int lsftp_get_opts( char* options, char* string );

int FATAL(char* msg);

#endif
