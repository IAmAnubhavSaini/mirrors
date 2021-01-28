/* connection.h
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

#ifndef LSH_CONNECTION_H_INCLUDED
#define LSH_CONNECTION_H_INCLUDED

#include "abstract_io.h"
#include "compress.h"
#include "queue.h"
#include "resource.h"
#include "randomness.h"


/* NOTE: CONNECTION_CLIENT and CONNECTION_SERVER are used both for
 * indexing the two-element arrays in the connection object, and in
 * the flags field, to indicate our role in the protocol. For
 * instance,
 *
 *   connection->versions[connection->flags & CONNECTION_MODE]
 *
 * is the version string we sent. Furthermore, install_keys() depends
 * on the numerical values of CONNECTION_SERVER and CONNECTION_CLIENT. */

enum connection_flag
  {
    CONNECTION_MODE = 1, 
    CONNECTION_CLIENT = 0,
    CONNECTION_SERVER = 1,
    CONNECTION_SRP = 2
  };

/* FIXME: Delete permanently? */
enum peer_flag
  {
    PEER_NONE = 0,
  };

/* State affecting incoming keyexchange packets */
enum kex_state
  {
    /* A KEX_INIT msg can be accepted. This is true, most of the
     * time. */
    KEX_STATE_INIT,

    /* Ignore next packet, whatever it is. */
    KEX_STATE_IGNORE,

    /* Key exchange is in progress. Neither KEX_INIT or NEWKEYS
     * messages, nor upper-level messages can be received. */
    KEX_STATE_IN_PROGRESS,

    /* Key exchange is finished. A NEWKEYS message should be received,
     * and nothing else. */
    KEX_STATE_NEWKEYS
  };

#define GABA_DECLARE
#include "connection.h.x"
#undef GABA_DECLARE

/* This is almost a write handler; difference is that it gets an extra
 * argument with a connection object. */

/* GABA:
   (class
     (name packet_handler)
     (vars
       (handler method void
               "struct ssh_connection *connection"
	       "struct lsh_string *packet")))
*/

#define HANDLE_PACKET(closure, connection, packet) \
((closure)->handler((closure), (connection), (packet)))

#define DEFINE_PACKET_HANDLER(SPEC, NAME, CARG, PARG)	\
static void						\
do_##NAME(struct packet_handler *,			\
	  struct ssh_connection *,			\
	  struct lsh_string *);				\
							\
SPEC struct packet_handler NAME =			\
{ STATIC_HEADER, do_##NAME };				\
							\
static void						\
do_##NAME(struct packet_handler *s UNUSED,		\
	  struct ssh_connection *CARG,			\
	  struct lsh_string *PARG)



/* GABA:
   (class
     (name ssh_connection)
     (super abstract_write)
     (vars
       ; Where to pass errors
       (e object exception_handler)

       ; Connection flags
       (flags . "enum connection_flag")

       ; Sent and received version strings
       (versions array (string) 2)
       (session_id string)
       
       ; Connection description, used for debug messages.
       (debug_comment . "const char *")

       ; Features or bugs peculiar to the peer
       (peer_flags . "enum peer_flag")

       ; Timer, used both for initial handshake timeout
       (timer object resource)
       
       ; Information about a logged in user. NULL unless some kind of
       ; user authentication has been performed.
       (user object lsh_user)

       ; The chained connection in the proxy, or gateway.
       (chain object ssh_connection)

       ; Cleanup
       (resources object resource_list)

       ; Connected peer and local
       ; FIXME: Perhaps this should be a sockaddr or some other object
       ; that facilitates reverse lookups?
       (peer object address_info)
       (local object address_info)

       ; Keyexchange
       (kexinit object make_kexinit)
       
       ; Receiving
       (rec_max_packet . uint32_t)
       (rec_mac    object mac_instance)
       (rec_crypto object crypto_instance)
       (rec_compress object compress_instance)

       ; Sending 
       (socket object lsh_fd)    ; Socket connected to the other end 
       ; When crossing the soft_limit we stop reading data on user
       ; channels, and set hard_limit to the current size plus some
       ; margin. When crossing the hard_limit, the connection is closed.
       (soft_limit . uint32_t)
       (hard_limit . uint32_t)

       (write_packet object abstract_write)  ; Where to send packets
       					     ; through the pipeline

       (send_mac object mac_instance)
       (send_crypto object crypto_instance)
       (send_compress object compress_instance)

       ; For operations that require serialization. In particular
       ; the server side of user authentication. 
       
       (paused . int)
       (pending struct string_queue)
       
       ; Key exchange 
       (read_kex_state . "enum kex_state")

       ; While in the middle of a key exchange, messages of most types
       ; must be queued up waiting for the key exchange to complete.
       (send_kex_only . int)
       (send_queue struct string_queue)
       
       ; For the key re-exchange logic
       (key_expire object resource)
       (sent_data . uint32_t)
       
       ; Invoked at the end of keyexchange.
       ; Automatically reset to zero after each invocation.
       ; Gets the connection as argument.
       (keyexchange_done object command_continuation)
       ; Called when key(re)exchange is finished, and when our
       ; write buffer shrinks anough to allow more channel data.
       (wakeup object command_continuation)
       
       (kexinits array (object kexinit) 2)
       (literal_kexinits array (string) 2)

       ; Table of all known message types 
       (dispatch array (object packet_handler) "0x100")
       
       ; Table of all opened channels
       (table object channel_table)
       
       ; (provides_privacy . int)
       ; (provides_integrity . int)
       ))
*/

struct ssh_connection *
make_ssh_connection(enum connection_flag flags,
		    struct address_info *peer,
		    struct address_info *local,
		    const char *id_comment,
		    struct exception_handler *e);

void
connection_init_io(struct ssh_connection *connection,
		   struct lsh_fd *socket);

void
connection_after_keyexchange(struct ssh_connection *self,
			     struct command_continuation *c);

void
connection_wakeup(struct ssh_connection *self,
		  struct command_continuation *c);

struct lsh_callback *
make_connection_close_handler(struct ssh_connection *c);

/* Processes the packet at once, passing it on to the write buffer. */
void
connection_send_kex(struct ssh_connection *self,
		    struct lsh_string *message);

/* Sending ordinary (non keyexchange) packets */
void
connection_send(struct ssh_connection *self,
		struct lsh_string *message);

void
connection_send_kex_start(struct ssh_connection *self);

void
connection_send_kex_end(struct ssh_connection *self);

/* Serialization */
void connection_lock(struct ssh_connection *self);
void connection_unlock(struct ssh_connection *self);

/* Timeouts */
void
connection_set_timeout(struct ssh_connection *connection,
		       unsigned seconds,
		       const char *msg);

void
connection_clear_timeout(struct ssh_connection *connection);

/* Table of packet types */
extern const char *packet_types[0x100];

/* Simple packet handlers. */
extern struct packet_handler connection_ignore_handler;
extern struct packet_handler connection_unimplemented_handler;
extern struct packet_handler connection_fail_handler;
extern struct packet_handler connection_forward_handler;
extern struct packet_handler connection_disconnect_handler;

/* Implemented in write_packet.h */
struct abstract_write *
make_write_packet(struct ssh_connection *connection,
		  struct randomness *random,
		  struct abstract_write *next);

/* Implemented in debug.c */
struct abstract_write *
make_packet_debug(struct abstract_write *continuation,
		  struct lsh_string *prefix);

void send_debug_message(struct ssh_connection *connection,
			const char *msg, int always_display);

extern struct packet_handler connection_debug_handler;

#endif /* LSH_CONNECTION_H_INCLUDED */
