/* debug.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "connection.h"
#include "format.h"
#include "parse.h"
#include "lsh_string.h"
#include "ssh.h"
#include "xalloc.h"
#include "werror.h"

#include "debug.c.x"

/* GABA:
   (class
     (name packet_debug)
     (super abstract_write_pipe)
     (vars
       (prefix string)))
*/

static void
do_debug(struct abstract_write *w,
	 struct lsh_string *packet)
{
  CAST(packet_debug, closure, w);

  if (!lsh_string_length(packet))
    debug("DEBUG: %S empty packet\n", closure->prefix);
  else
    {
      uint8_t type = lsh_string_data(packet)[0];
      if (type == SSH_MSG_USERAUTH_REQUEST
	  || type == SSH_MSG_USERAUTH_INFO_RESPONSE)
	debug("DEBUG: %S %z *****\n",
	      closure->prefix, packet_types[type]);
      else
	debug("DEBUG: %S %z %xS\n",
	      closure->prefix, packet_types[type],
	      packet);
    }
  A_WRITE(closure->super.next, packet);
}

struct abstract_write *
make_packet_debug(struct abstract_write *next,
		  struct lsh_string *prefix)
{
  NEW(packet_debug, closure);

  closure->super.super.write = do_debug;
  closure->super.next = next;
  closure->prefix = prefix;

  return &closure->super.super;
}


static struct lsh_string *
make_debug_packet(const char *msg, int always_display)
{
  return ssh_format("%c%c%z%z",
		    SSH_MSG_DEBUG,
		    always_display,
		    msg,
		    /* Empty language tag */ 
		    "");
}

/* Send a debug message to the other end. */
void
send_debug_message(struct ssh_connection *connection,
		   const char *msg, int always_display)
{
  /* Can be sent even during key exchange. */
  connection_send_kex(connection, make_debug_packet(msg, always_display));
}

DEFINE_PACKET_HANDLER(, connection_debug_handler, connection UNUSED, packet)
{
  struct simple_buffer buffer;
  unsigned msg_number;
  unsigned always_display;
  uint32_t length;
  const uint8_t *msg;
  int language;
  
  simple_buffer_init(&buffer, STRING_LD(packet));

  if (!(parse_uint8(&buffer, &msg_number)
	&& parse_uint8(&buffer, &always_display)
	&& parse_string(&buffer, &length, &msg)
	&& parse_atom(&buffer, &language)
	&& parse_eod(&buffer)))
    {
      PROTOCOL_ERROR(connection->e, "Invalid DEBUG message.");
    }
  else
    {
      if (always_display)
	werror("Received debug: %ups\n", length, msg);

      else
	verbose("Received debug: %ups\n", length, msg);
    }
}
