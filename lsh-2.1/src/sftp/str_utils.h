/*
 * @(#) $Id: 0772e7471bf61828823abb6dcdfbb65db4f32264 $
 *
 * str_utils.h
 */

/* lsftp, an implementation of the sftp protocol
 *
 * Copyright (C) 2001, Pontus Sköld
 * Portions of this code originately from the readline manual
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

#ifndef LSFTP_STR_UTILS_H
#define LSFTP_STR_UTILS_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Quote and unqoute a string */
char* lsftp_quote( const char* s );
char* lsftp_unquote( const char* s );

/* Given a string and a set of tokens, place in store (is of size storelen) */
const char *lsftp_s_strtok(const char *s, const char* tok,
			   char **store); 

const char *lsftp_s_skip(const char *s, const char *sep);
const char* lsftp_s_skipn(const char *s, const char *sep );

#endif
