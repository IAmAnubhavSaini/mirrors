/* write_packet.c */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2003 Niels Möller
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

#include <assert.h>

#include <nettle/macros.h>

#include "connection.h"
#include "format.h"
#include "lsh_string.h"
#include "werror.h"
#include "xalloc.h"

#include "write_packet.c.x"

/* GABA:
   (class
     (name write_packet)
     (super abstract_write_pipe)
     (vars
       (connection object ssh_connection)
       (random object randomness)
       (sequence_number . uint32_t)))
*/

static void
do_write_packet(struct abstract_write *s,
		struct lsh_string *packet)
{
  CAST(write_packet, self, s);
  struct ssh_connection *connection = self->connection;
  uint32_t block_size;
  uint32_t new_size;
  uint8_t padding_length;
  uint32_t padding;

  uint32_t mac_length;
  uint32_t mac;

  uint32_t length = lsh_string_length(packet);
  assert(length);
  
  /* Deflate, pad, mac, encrypt. */
  if (connection->send_compress)
    {
      packet = CODEC(connection->send_compress, packet, 1);
      assert(packet);
      length = lsh_string_length(packet);      
    }

  block_size = connection->send_crypto
    ? connection->send_crypto->block_size : 8;
  mac_length = connection->send_mac
    ? connection->send_mac->mac_size : 0;
  
  /* new_size is (length + 9) rounded up to a multiple of
   * block_size */
  new_size = block_size * (1 + (8 + length) / block_size);
  
  padding_length = new_size - length - 5;
  assert(padding_length >= 4);

  packet = ssh_format("%i%c%lfS%lr%lr",
		      length + padding_length + 1,
		      padding_length,
		      packet,
		      padding_length, &padding,
		      mac_length, &mac);

  assert(new_size + mac_length == lsh_string_length(packet));

  lsh_string_write_random(packet, padding, self->random, padding_length);

  if (connection->send_mac)
    {
      uint8_t s[4];
      assert(new_size == mac);      

      WRITE_UINT32(s, self->sequence_number);
      MAC_UPDATE(connection->send_mac, 4, s);
      MAC_UPDATE(connection->send_mac, new_size, lsh_string_data(packet));
      MAC_DIGEST(connection->send_mac, packet, mac);
    }
  if (connection->send_crypto)
    CRYPT(connection->send_crypto, new_size, packet, 0, packet, 0);

  self->sequence_number++;
  A_WRITE(self->super.next, packet);
}

struct abstract_write *
make_write_packet(struct ssh_connection *connection,
		  struct randomness *random,
		  struct abstract_write *next)
{
  NEW(write_packet, self);
  self->super.super.write = do_write_packet;
  self->super.next = next;
  self->connection = connection;
  self->random = random;
  self->sequence_number = 0;

  return &self->super.super;
}
