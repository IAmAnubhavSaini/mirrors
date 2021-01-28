/*
 * @(#) $Id: 61db129c99c21ac745bdf39dd67d4ed400c5369b $
 *
 * commands.h
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


#ifndef LSFTP_COMMANDS_H
#define LSFTP_COMMANDS_H
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <glob.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include "sftp_bind.h"
#include "dc.h"
#include "str_utils.h"
#include "misc_fun.h"
#include "gnu_stuff.h"

#define NOCOMMAND -4710
#define UNKNOWNCOMMAND -4711

#define WORDLENMAX 256

extern int mainloop; /* Flag to end mainloop */
extern int sloppy_complete;
extern int buggy_server_treshold;

/* Public functions */

int handle_command(const unsigned char *s);


/* Internal below */

int com_help(const char *arg, const char *command);
int com_longhelp(const char *arg, const char *command);
int com_about(const char *arg, const char *command);
int com_escape(const char* arg, const char *command);
int com_quit(const char *arg, const char *command);
int com_jobs(const char *arg, const char *command);

int com_close(const char *arg, const char *command);
int com_open(const char *arg, const char *command);

int com_cd(const char *arg, const char *command);
int com_rm(const char *arg, const char *command);
int com_ls(const char *arg, const char *command);
int com_pwd(const char *arg, const char *command);
int com_mv(const char *arg, const char *command);

int com_ln(const char *arg, const char *command);

int com_mkdir(const char *arg, const char *command);

int com_set(const char *arg, const char *command);
int com_mail(const char *arg, const char *command);
int com_umask(const char *arg, const char *command);

int com_get(const char *arg, const char *command);
int com_put(const char *arg, const char *command);

int com_lcd(const char *arg, const char *command);
int com_lrm(const char *arg, const char *command);
int com_lls(const char *arg, const char *command);
int com_lpwd(const char *arg, const char *command);
int com_lmv(const char *arg, const char *command);

int com_chown(const char *arg, const char *command);
int com_chgrp(const char *arg, const char *command);

enum takes_as_arg
{ 
  COMMAND,
  LOCALFILE,
  REMOTEFILE,
  NOARG,
  JOBID,
  OTHERARG
};

typedef int (*command_func)(const char *arg, const char *command);
typedef struct 
{
  char *name;                   /* User printable name of the function. */
  command_func func;            /* Function to call to do the job. */
  char *doc;                    /* Documentation for this function.  */
  char* longdoc;                /* Longer help to display for help command */
  enum takes_as_arg arg_type;
  int visible;
  int uniquelen;
} command;

extern command commands[];

#endif










