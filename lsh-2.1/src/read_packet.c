/* read_packet.c
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

#include <assert.h>
#include <string.h>

#include <nettle/macros.h>

#include "read_packet.h"

#include "crypto.h"
#include "format.h"
#include "io.h"
#include "lsh_string.h"
#include "ssh.h"
#include "werror.h"
#include "xalloc.h"

#define WAIT_START 0
#define WAIT_HEADER 1
#define WAIT_CONTENTS 2
#define WAIT_MAC 3

#include "read_packet.c.x"


/* GABA:
   (class
     (name read_packet)
     (super read_handler)
     (vars
       (state . int)
  
       ; Attached to read packets
       (sequence_number . uint32_t)
  
       ; Buffer index, used for all the buffers
       (pos . uint32_t)

       ; Buffer for first received block
       (block_buffer string)
       ; Buffer for received MAC
       (mac_buffer string) 
       ; Computed MAC
       (mac_computed string)
       
       ; Holds the packet payload
       (packet_buffer string)

       ; Length without padding
       (payload_length . uint32_t)
       
       ; Position in the buffer after the first,
       ; already decrypted, block.
       (crypt_pos . uint32_t)
  
       (handler object abstract_write)
       (connection object ssh_connection)))
*/


#define READ(n, dst) do {				\
  lsh_string_write((dst), closure->pos, (n), data);	\
  closure->pos += (n);					\
  data += (n);						\
  total += (n);						\
  available -= (n);					\
} while (0)

static uint32_t
do_read_packet(struct read_handler **h,
	       uint32_t available,
	       const uint8_t *data)
{
  CAST(read_packet, closure, *h);
  uint32_t total = 0;

  if (!available)
    {
      debug("read_packet: EOF in state %i\n", closure->state);

      if (closure->state != WAIT_START)
        EXCEPTION_RAISE(closure->connection->e,
                        make_protocol_exception(0, "Unexpected EOF"));
      else
        /* FIXME: This may still be "unexpected".
         *
         * We should check that there are no open channels. */
        EXCEPTION_RAISE(closure->connection->e, &finish_read_exception);
      
      *h = NULL;
      return 0;
    }
	  
  for (;;)
    switch(closure->state)
      {
      case WAIT_START:
	assert(! closure->connection->rec_crypto
	       || closure->connection->rec_crypto->block_size <= SSH_MAX_BLOCK_SIZE); 
	assert(! closure->connection->rec_mac
	       || closure->connection->rec_mac->mac_size <= SSH_MAX_MAC_SIZE);

        closure->state = WAIT_HEADER;
	closure->pos = 0;
	/* FALL THROUGH */
	  
      case WAIT_HEADER:
	{
	  uint32_t block_size = closure->connection->rec_crypto
	    ? closure->connection->rec_crypto->block_size : 8;
	  uint32_t left;

	  left = block_size - closure->pos;
	  assert(left);

	  if (available < left)
	    {
	      READ(available, closure->block_buffer);

	      return total;
	    }
	  else
	    {
	      /* We have read a complete block */
	      uint32_t length;
	      const uint8_t *block;
	      uint8_t pad_length;

	      READ(left, closure->block_buffer);
	    
	      if (closure->connection->rec_crypto)
		CRYPT(closure->connection->rec_crypto,
		      block_size,
		      closure->block_buffer, 0,
		      closure->block_buffer, 0);

	      block = lsh_string_data(closure->block_buffer);
	      length = READ_UINT32(block);

	      /* NOTE: We don't implement a limit at _exactly_
	       * rec_max_packet, as we don't include the length field
	       * and MAC in the comparison below. */
	      if (length > (closure->connection->rec_max_packet + SSH_MAX_PACKET_FUZZ))
		{
		  static const struct protocol_exception too_large =
		    STATIC_PROTOCOL_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
					      "Packet too large");
		  
		  werror("read_packet: Receiving too large packet.\n"
			 "  %i octets, limit is %i\n",
			 length, closure->connection->rec_max_packet);
		  
		  EXCEPTION_RAISE(closure->connection->e, &too_large.super);

		  return total;
		}

	      if ( (length < 12)
		   || (length < (block_size - 4))
		   || ( (length + 4) % block_size))
		{
		  static const struct protocol_exception invalid =
		    STATIC_PROTOCOL_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
					      "Invalid packet length");
		  
		  werror("read_packet: Bad packet length %i\n",
			 length);
		  EXCEPTION_RAISE(closure->connection->e, &invalid.super);

		  return total;
		}

	      /* Process this block before the length field is lost. */
	      if (closure->connection->rec_mac)
		{
		  uint8_t s[4];
		  WRITE_UINT32(s, closure->sequence_number);
		  MAC_UPDATE(closure->connection->rec_mac, 4, s);
		  MAC_UPDATE(closure->connection->rec_mac,
			     block_size, block);
		}

	      /* Extract padding length */
	      pad_length = block[4];

	      debug("do_read_packet: length = %i, pad_length = %i\n",
		    length, pad_length);
	      
	      length--;

	      if ( (pad_length < 4)
		   || (pad_length >= length) )
		{
		  PROTOCOL_ERROR(closure->connection->e,
				 "Bogus padding length.");
		  return total;
		}

	      closure->payload_length = length - pad_length;
	      
	      /* Allocate full packet */
	      {
		unsigned done = block_size - 5;

		assert(!closure->packet_buffer);
		
		closure->packet_buffer
		  = ssh_format("%ls%lr",
			       done, block + 5,
			       length - done, &closure->crypt_pos);

		assert(lsh_string_length(closure->packet_buffer) == length);
		
		/* The sequence number is needed by the handler for
		 * unimplemented message types. */
		lsh_string_set_sequence_number(closure->packet_buffer,
					       closure->sequence_number ++);
		closure->pos = done;

		if (done == length)
		  {
		    /* A complete ssh packet fitted in the first
		     * encryption block. */
		    debug("read_packet.c: "
			  "Going directly to the WAIT_MAC state\n");

		    goto do_mac;
		  }
		else
		  goto do_contents;
	      }
	    }
	}
	fatal("read_packet: Supposedly not happening???\n");
	  
      do_contents:
        closure->state = WAIT_CONTENTS;
	
      case WAIT_CONTENTS:
	{
	  uint32_t length = lsh_string_length(closure->packet_buffer);
	  uint32_t left = length - closure->pos;

	  assert(left);

	  if (available < left)
	    {
	      READ(available, closure->packet_buffer);
	    
	      return total;
	    }
	  else
	    {
	      /* Read a complete packet */
	      READ(left, closure->packet_buffer);

	      left = length - closure->crypt_pos;

	      if (closure->connection->rec_crypto)
		CRYPT(closure->connection->rec_crypto,
		      left,
		      closure->packet_buffer, closure->crypt_pos,
		      closure->packet_buffer, closure->crypt_pos);		      

	      if (closure->connection->rec_mac)
		MAC_UPDATE(closure->connection->rec_mac,
			   left,
			   lsh_string_data(closure->packet_buffer)
			   + closure->crypt_pos);

	      goto do_mac;
	    }
	}
	fatal("read_packet: Supposedly not happening???\n");

      do_mac:
        closure->state = WAIT_MAC;
	closure->pos = 0;
      
      case WAIT_MAC:
	/* NOTE: It would be possible to first compute the expected
	   MAC, and then compare bytes as they are read. But we don't
	   want to tell an attacker that the MAC was wrong until we
	   have received all the bytes, to avoid information leakage.
	   And then it seems easier to read the entire MAC first and
	   examine it later. */
	
	if (closure->connection->rec_mac)
	  {
	    uint32_t mac_size = closure->connection->rec_mac->mac_size;
	    uint32_t left = mac_size - closure->pos;

	    assert(left);

	    if (available < left)
	      {
		READ(available, closure->mac_buffer);
	      
		return total;
	      }
	    else
	      {
		/* Read a complete MAC */
		READ(left, closure->mac_buffer);

		MAC_DIGEST(closure->connection->rec_mac, closure->mac_computed, 0);

		if (memcmp(lsh_string_data(closure->mac_buffer),
			   lsh_string_data(closure->mac_computed),
			   mac_size))
		  {
		    static const struct protocol_exception mac_error =
		      STATIC_PROTOCOL_EXCEPTION(SSH_DISCONNECT_MAC_ERROR,
						"MAC error");
		    EXCEPTION_RAISE(closure->connection->e, &mac_error.super);
		    return total;
		  }
	      }
	  }
	
	/* MAC was ok, send packet on */
	{
	  struct lsh_string *packet = closure->packet_buffer;
	  
	  closure->packet_buffer = NULL;
	  closure->state = WAIT_START;

	  /* Strip padding */
	  lsh_string_trunc(packet, closure->payload_length);
	  
	  if (closure->connection->rec_compress)
	    {
	      uint32_t sequence_number = lsh_string_sequence_number(packet);
	      packet = CODEC(closure->connection->rec_compress, packet, 1);

	      if (!packet)
		{
		  /* FIXME: It would be nice to pass the error message from zlib on
		   * to the exception handler. */
		  EXCEPTION_RAISE
		    (closure->connection->e,
		     make_protocol_exception(SSH_DISCONNECT_COMPRESSION_ERROR,
					     "Inflating compressed data failed."));
		  return total;
		}
	      /* Keep sequence number */
	      lsh_string_set_sequence_number(packet, sequence_number);
	    }
	      
	  A_WRITE(closure->handler, packet);
	  return total;
	}
      default:
	fatal("Internal error\n");
    }
}

struct read_handler *
make_read_packet(struct abstract_write *handler,
		 struct ssh_connection *connection)
{
  NEW(read_packet, closure);

  closure->super.handler = do_read_packet;

  closure->connection = connection;
  closure->handler = handler;

  closure->state = WAIT_START;
  closure->sequence_number = 0;

  closure->block_buffer = lsh_string_alloc(SSH_MAX_BLOCK_SIZE);
  closure->mac_buffer = lsh_string_alloc(SSH_MAX_MAC_SIZE);
  closure->mac_computed = lsh_string_alloc(SSH_MAX_MAC_SIZE);
  closure->packet_buffer = NULL;
  
  return &closure->super;
}
