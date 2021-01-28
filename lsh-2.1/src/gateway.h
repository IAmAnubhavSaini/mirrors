/* gateway.h
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2000 Niels M�ller
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

#ifndef GATEWAY_H_INCLUDED
#define GATEWAY_H_INCLUDED

/* FIXME: Do we really need this file? */

#include "lsh.h"

/* Formats the address of the local gateway socket. */

struct local_info *
make_gateway_address(const char *local_user, const char *remote_user,
		     const char *target);

#endif /* GATEWAY_H_INCLUDED */
