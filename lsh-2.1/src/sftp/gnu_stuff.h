/*
 * @(#) $Id: a978ac24f162a55079cf244b02ef126e92f2ee72 $
 *
 * gnu_stuff.h
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

#ifndef LSFTP_GNU_STUFF_H
#define LSFTP_GNU_STUFF_H

#include <stdio.h>

void help_option(void);
void version_option(void);
void lsftp_welcome(void); 

void do_gnu_stuff( const char** argv );


#ifndef HAVE_CANONICALIZE_FILE_NAME
char* canonicalize_file_name (const char* name);
#endif

#endif /* LSFTP_GNU_STUFF_H */
