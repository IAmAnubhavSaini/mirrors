/* server_x11.h
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2002 Niels Möller
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef LSH_SERVER_X11_H_INCLUDED
#define LSH_SERVER_X11_H_INCLUDED

#include "channel.h"
#include "resource.h"

#define GABA_DECLARE
#include "server_x11.h.x"
#undef GABA_DECLARE

/* GABA:
   (class
     (name server_x11_info)
     (vars
       (display string)
       (xauthority string)))
*/

struct server_x11_info *
server_x11_setup(struct ssh_channel *channel, struct lsh_user *user,
		 int single,
		 uint32_t protocol_length, const uint8_t *protocol,
		 uint32_t cookie_length, const uint8_t *cookie,
		 uint32_t screen,
		 struct command_continuation *c,
		 struct exception_handler *e);


#endif /* LSH_SERVER_X11_H_INCLUDED */
