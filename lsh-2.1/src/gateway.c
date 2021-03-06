/* gateway.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "gateway.h"

#include "environ.h"
#include "format.h"
#include "io.h"

/* A gateway is a mechanism to delegate some channels to a separate
 * process. The main lsh process opens a unix domain socket, and other
 * processes can connect and read and write cleartext ssh packets.
 * Packets written to the gateway socket are forwarded to the remote
 * server. And certain packets received from the remote server are
 * delegated and can be read from the gateway. */


/* The gateway socket is named "TMP/x-lsh-USER/HOST%REMOTE-USER".
 *
 * The choice of the '%' separator in the socket name makes sure we
 * don't collide with any valid dns names, or with literal IPv4 or IPv6
 * addresses. And it should be really rare in usernames. */

static int
check_string_l(unsigned length, const uint8_t *s)
{
  unsigned i;
  for (i = 0; i<length; i++)
    switch(*s++)
      {
      case  '\0':
      case '%':
      case '/':
	return 0;
      default:
	break;
      }
  return 1;
}

static int
check_string(const uint8_t *s)
{
  for (;;)
    switch(*s++)
      {
      case  '\0':
	return 1;
      case '%':
      case '/':
	return 0;
      default:
	break;
      }
}

struct local_info *
make_gateway_address(const char *local_user, const char *remote_user,
		     const char *target)
{
  char *tmp = getenv(ENV_TMPDIR);
  unsigned length = strlen(target);
  if (!tmp)
    tmp = "/tmp";
  
  if (check_string(local_user)
      && check_string(remote_user)
      && check_string_l(length, target))
    return make_local_info(ssh_format("%lz/x-lsh-%lz", tmp, local_user),
			   ssh_format("%lz:%lz", target, remote_user));
  else
    return NULL;
}

/* Keeps track of one connection to the gateway. */

/* ;;GABA:
   (class
     (name gateway)
     (vars
       ; Where to send packets
       (local object abstract_write)))
*/

