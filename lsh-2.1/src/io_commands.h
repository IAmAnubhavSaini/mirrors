/* io_commands.h
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998 Niels Möller
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

#ifndef LSH_IO_COMMANDS_H_INCLUDED
#define LSH_IO_COMMANDS_H_INCLUDED

#include "connection.h"
#include "io.h"

extern struct command_2 listen_command;
#define LISTEN (&listen_command.super.super)

extern struct command bind_address_command;
#define BIND (&bind_address_command.super)

extern struct command bind_local_command;
#define BIND_LOCAL (&bind_local_command.super)


struct command *
make_connect_port(struct address_info *target);

extern struct command_2 connect_connection_command;
#define CONNECT_CONNECTION (&connect_connection_command.super.super)

extern struct command connect_simple_command;
#define CONNECT_SIMPLE (&connect_simple_command.super)

extern struct command connect_list_command;
#define CONNECT_LIST (&connect_list_command.super)

extern struct command connect_local_command;
#define CONNECT_LOCAL (&connect_local_command.super)

extern struct command io_log_peer_command;
#define LOG_PEER (&io_log_peer_command.super)


struct command *
make_tcp_wrapper(struct lsh_string *name, struct lsh_string *msg);

#endif /* LSH_IO_COMMANDS_H_INCLUDED */
