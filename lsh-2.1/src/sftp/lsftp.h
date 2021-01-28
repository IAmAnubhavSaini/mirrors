/*
 * @(#) $Id: 52a3c000f0fe600b6fbb9d658989d49eb6a0977c $
 *
 * lsftp.h
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

#ifndef LSFTP_LSFTP_H
#define LSFTP_LSFTP_H

#include <stdio.h>
#include "sftp_bind.h"

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "rl.h"
#include "str_utils.h"
#include "gnu_stuff.h"

extern int lsftp_rl_line_done;
extern char* lsftp_rl_line;

#define LSFTP_COMMAND_MARKER "--"                             /* Used to separate ssh-parameters and lsftp commands */

#endif
