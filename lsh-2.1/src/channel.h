/* channel.h
 *
 * Information about ssh channels.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998 Niels M�ller
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

#ifndef LSH_CHANNEL_H_INCLUDED
#define LSH_CHANNEL_H_INCLUDED

#include "alist.h"
#include "command.h"
#include "connection.h"
#include "parse.h"
#include "server_pty.h"
#include "write_buffer.h"

struct channel_open_info
{
  uint32_t type_length;

  /* NOTE: This is a pointer into the packet, so if it is needed later
   * it must be copied. */
  const uint8_t *type_data;
  
  int type;

  uint32_t remote_channel_number;
  uint32_t send_window_size;
  uint32_t send_max_packet;
};

struct channel_request_info
{
  uint32_t type_length;
  const uint8_t *type_data;
  
  int type;

  int want_reply;
};

#define GABA_DECLARE
#include "channel.h.x"
#undef GABA_DECLARE

/* Channels are indexed by local channel number in some array. This
 * index is not stored in the channel struct. When sending messages on
 * the channel, it is identified by the *remote* sides index number,
 * and this number must be stored. */

#define CHANNEL_DATA 0
#define CHANNEL_STDERR_DATA 1

#define CHANNEL_SENT_CLOSE 1
#define CHANNEL_RECEIVED_CLOSE 2
#define CHANNEL_SENT_EOF 4
#define CHANNEL_RECEIVED_EOF 8

/* Normally, this flag is set, and we initiate channel close as soon
 * as we have both sent and received SSH_MSG_CHANNEL_EOF. Clearing
 * this flag keeps the channel open. */

#define CHANNEL_CLOSE_AT_EOF 0x10

/* This flags means that we don't expect any more data from the other
 * end, and that we don't want to wait for an SSH_MSG_CHANNEL_EOF
 * before closing the channel. */

#define CHANNEL_NO_WAIT_FOR_EOF 0x20

/* GABA:
   (class
     (name ssh_channel)
     (super flow_controlled)
     (vars
       ; Remote channel number 
       (channel_number . uint32_t)

       ; Where to pass errors. This is used for two different
       ; purposes: If opening the channel fails, EXC_CHANNEL_OPEN is
       ; raised. Once the channel is open, this handler is used for
       ; EXC_FINISH_CHANNEL and EXC_FINISH_PENDING. If the channel was
       ; opened on the peer's request, the connection's exception
       ; handler is a parent of the channel's. But that is not true in
       ; general.
       (e object exception_handler)

       ; Resources associated with the channel. This object is also
       ; put onto the connections resource list.
       (resources object resource_list)
       
       ; NOTE: The channel's maximum packet sizes refer to the packet
       ; payload, i.e. the DATA string in SSH_CHANNEL_DATA and
       ; SSH_MSG_CHANNEL_EXTENDED_DATA.

       (rec_window_size . uint32_t)
       (rec_max_packet . uint32_t)

       (send_window_size . uint32_t)
       (send_max_packet . uint32_t)

       (connection object ssh_connection)
       
       (request_types object alist)

       ; If non-NULL, invoked for unknown channel requests.
       (request_fallback object channel_request)
       
       (flags . int)

       ; Number of files connected to this channel. For instance,
       ; stdout and stderr can be multiplexed on the same channel. We
       ; should not close the channel until we have got an EOF on both
       ; sources.
       (sources . int)

       ; Type is CHANNEL_DATA or CHANNEL_STDERR_DATA
       (receive method void "int type" "struct lsh_string *data")

       ; Called when we are allowed to send more data on the channel.
       ; Implies that the send_window_size is non-zero. 
       (send_adjust method void "uint32_t increment")

       ; Called when the channel is closed.
       ; Used by client_session and gateway_channel.
       (close method void)

       ; Called when eof is received on the channel (or when it is
       ; closed, whatever happens first).
       (eof method void)
  
       ; Reply from SSH_MSG_CHANNEL_OPEN_REQUEST
       (open_continuation object command_continuation)

       ; Queue of channel requests that we expect replies on
       (pending_requests struct object_queue)

       ; Channel requests that we have received, and should reply to
       ; in the right order
       (active_requests struct object_queue)))
       
*/

#define CHANNEL_RECEIVE(s, t, d) \
((s)->receive((s), (t), (d)))

#define CHANNEL_SEND_ADJUST(s, i) ((s)->send_adjust((s), (i)))
     
#define CHANNEL_CLOSE(s) \
((s)->close((s)))

#define CHANNEL_EOF(s) \
((s)->eof((s)))

#define CHANNEL_OPEN_CONFIRM(s) \
((s)->open_confirm((s)))

#define CHANNEL_OPEN_FAILURE(s) \
((s)->open_failure((s)))

/* Values used in the in_use array. */
#define CHANNEL_FREE 0
#define CHANNEL_RESERVED 1
#define CHANNEL_IN_USE 2

/* GABA:
   (class
     (name channel_table)
     (vars
       ; Channels are indexed by local number
       (channels space (object ssh_channel) used_channels)
       
       ; Global requests that we support
       (global_requests object alist)
       ; Channel types that we can open
       (channel_types object alist)

       ; Used for unknown requests unknown channel types.
       (open_fallback object channel_open)
       
       ; Allocation of local channel numbers is managed using the same
       ; method as is traditionally used for allocation of unix file 
       ; descriptors.

       ; Channel numbers can be reserved before there is any actual
       ; channel assigned to them. So the channels table is not enough
       ; for keeping track of which numbers are in use.
       (in_use space uint8_t)

       ; Allocated size of the arrays.
       (allocated_channels . uint32_t)

       ; Number of entries in the arrays that are in use and
       ; initialized.
       (used_channels . uint32_t)

       ; The smallest channel number that is likely to be free
       (next_channel . uint32_t)

       ; Number of currently allocated channel numbers.
       (channel_count . uint32_t)
       
       (max_channels . uint32_t) ; Max number of channels allowed 

       ; Forwarded TCP ports
       (local_ports struct object_queue)
       (remote_ports struct object_queue)

       ; Used if we're currently forwarding X11
       ; To support several screens at the same time,
       ; this should be replaced with a list, analogous to
       ; the remote_ports list above.
       (x11_display object client_x11_display)
       
       ; Global requests that we have received, and should reply to
       ; in the right order
       (active_global_requests struct object_queue)

       ; Queue of global requests that we expect replies on.
       (pending_global_requests struct object_queue)
       
       ; If non-zero, close connection after all active channels have
       ; died, and don't allow any new channels to be opened.
       (pending_close . int)))
*/

/* SSH_MSG_GLOBAL_REQUEST */

/* GABA:
   (class
     (name global_request)
     (vars
       (handler method void "struct ssh_connection *connection"
                            "uint32_t type"
			    ; want-reply is needed only by
			    ; do_gateway_global_request.
                            "int want_reply"
                            "struct simple_buffer *args"
			    "struct command_continuation *c"
			    "struct exception_handler *e")))
*/

#define GLOBAL_REQUEST(r, c, t, w, a, n, e) \
((r)->handler((r), (c), (t), (w), (a), (n), (e)))

/* SSH_MSG_CHANNEL_OPEN */
  
/* Raised if opening of a channel fails. Used both on the client and
 * the server side.*/
/* GABA:
   (class
     (name channel_open_exception)
     (super exception)
     (vars
       (error_code . uint32_t)))
*/

struct exception *
make_channel_open_exception(uint32_t error_code, const char *msg);


/* GABA:
   (class
     (name channel_open)
     (vars
       (handler method void
                "struct ssh_connection *connection"
		"struct channel_open_info *info"
                "struct simple_buffer *data"
                "struct command_continuation *c"
		"struct exception_handler *e")))
*/

#define CHANNEL_OPEN(o, c, i, d, r, e) \
((o)->handler((o), (c), (i), (d), (r), (e)))

#define DEFINE_CHANNEL_OPEN(name)                       \
static void do_##name(struct channel_open *s,           \
		     struct ssh_connection *connection, \
		     struct channel_open_info *info,    \
		     struct simple_buffer *args,        \
		     struct command_continuation *c,    \
		     struct exception_handler *e);      \
                                                        \
struct channel_open name =                              \
{ STATIC_HEADER, do_##name };                           \
                                                        \
static void do_##name

/* SSH_MSG_CHANNEL_REQUEST */

/* GABA:
   (class
     (name channel_request)
     (vars
       (handler method void
		"struct ssh_channel *channel"
		"struct channel_request_info *info"
		"struct simple_buffer *args"
		"struct command_continuation *c"
		"struct exception_handler *e")))
*/

#define CHANNEL_REQUEST(s, c, i, a, n, e) \
((s)->handler((s), (c), (i), (a), (n), (e)))

#define DEFINE_CHANNEL_REQUEST(name)                            \
static void do_##name(struct channel_request *s,                \
		      struct ssh_channel *channel,              \
                      struct channel_request_info *info,        \
		      struct simple_buffer *args,               \
		      struct command_continuation *c,           \
		      struct exception_handler *e);             \
                                                                \
struct channel_request name =                                   \
{ STATIC_HEADER, do_##name };                                   \
                                                                \
static void do_##name

void init_channel(struct ssh_channel *channel);

struct channel_table *make_channel_table(void);
int alloc_channel(struct channel_table *table);
void dealloc_channel(struct channel_table *table, int i);

void
use_channel(struct ssh_connection *connection,
	    uint32_t local_channel_number);

void
register_channel(uint32_t local_channel_number,
		 struct ssh_channel *channel,
		 int take_into_use);

struct ssh_channel *
lookup_channel(struct channel_table *table, uint32_t i);
struct ssh_channel *
lookup_channel_reserved(struct channel_table *table, uint32_t i);

struct abstract_write *make_channel_write(struct ssh_channel *channel);
struct abstract_write *make_channel_write_extended(struct ssh_channel *channel,
						   uint32_t type);

struct io_callback *make_channel_read_data(struct ssh_channel *channel);
struct io_callback *make_channel_read_stderr(struct ssh_channel *channel);

struct lsh_string *format_global_failure(void);
struct lsh_string *format_global_success(void);

struct lsh_string *format_open_failure(uint32_t channel, uint32_t reason,
				       const char *msg, const char *language);
struct lsh_string *format_open_confirmation(struct ssh_channel *channel,
					    uint32_t channel_number,
					    const char *format, ...);

struct lsh_string *format_channel_success(uint32_t channel);
struct lsh_string *format_channel_failure(uint32_t channel);

struct lsh_string *prepare_window_adjust(struct ssh_channel *channel,
					 uint32_t add);

void
channel_start_receive(struct ssh_channel *channel,
		      uint32_t initial_window_size);

struct lsh_string *
format_channel_open_s(struct lsh_string *type,
		      uint32_t local_channel_number,
		      struct ssh_channel *channel,
		      struct lsh_string *args);

struct lsh_string *
format_channel_open(int type, uint32_t local_channel_number,
		    struct ssh_channel *channel,
		    const char *format, ...);

struct lsh_string *
format_channel_request_i(struct channel_request_info *info,
			 struct ssh_channel *channel,
			 uint32_t args_length, const uint8_t *args_data);

struct lsh_string *
format_channel_request(int type,
		       struct ssh_channel *channel,
		       int want_reply,
		       const char *format, ...);

struct lsh_string *
format_global_request(int type, int want_reply,
		      const char *format, ...);

struct lsh_string *format_channel_close(struct ssh_channel *channel);
struct lsh_string *format_channel_eof(struct ssh_channel *channel);

void channel_close(struct ssh_channel *channel);
void channel_eof(struct ssh_channel *channel);

struct lsh_callback *
make_channel_read_close_callback(struct ssh_channel *channel);

struct exception_handler *
make_channel_io_exception_handler(struct ssh_channel *channel,
				  const char *prefix,
				  int silent,
				  struct exception_handler *parent,
				  const char *context);

struct lsh_string *channel_transmit_data(struct ssh_channel *channel,
					 struct lsh_string *data);

struct lsh_string *channel_transmit_extended(struct ssh_channel *channel,
					     uint32_t type,
					     struct lsh_string *data);

void init_connection_service(struct ssh_connection *connection);
extern struct command connection_service_command;
#define INIT_CONNECTION_SERVICE (&connection_service_command.super)

#if 0
void init_login_service(struct ssh_connection *connection);
extern struct command login_service_command;
#define INIT_LOGIN_SERVICE (&login_service_command.super)
#endif

#endif /* LSH_CHANNEL_H_INCLUDED */
