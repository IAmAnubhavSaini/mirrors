/* io.h
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998, 2001 Niels Möller
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

#ifndef LSH_IO_H_INCLUDED
#define LSH_IO_H_INCLUDED

#include <time.h>
#include <netdb.h>
/* For sig_atomic_t */
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "abstract_io.h"
#include "queue.h"
#include "resource.h"
#include "write_buffer.h"

enum io_type {
  IO_NORMAL = 0,

  /* The type IO_PTY is the master side of a pty pair. It is handled
     specially by close_fd_write. */
  IO_PTY = 1,

  /* Stdio file desciptors are special, because they're in blocking
     mode, and when closed, we must open /dev/null to avoid accidental
     reuse of the special fd:s. */
  IO_STDIO = 2,
  
  /* Blocking mode, and never closed. */
  IO_STDERR = 3,
};

/* Max number of simultaneous connection attempts */
#define CONNECT_ATTEMPTS_LIMIT 3

#define GABA_DECLARE
#include "io.h.x"
#undef GABA_DECLARE


/* GABA:
   (class
     (name lsh_callback)
     (vars
       (f method void)))
*/

#define LSH_CALLBACK(c) ((c)->f((c)))

/* The fd io callback is a closure, in order to support different
 * reading styles (buffered and consuming). Also used for writing. */

/* GABA:
   (class
     (name io_callback)
     (vars
       (f method void "struct lsh_fd *fd")))
*/

#define IO_CALLBACK(c, fd) ((c)->f((c), (fd)))

/* GABA:
   (class
     (name lsh_fd)
     (super resource)
     (vars
       (fd . int)
       ; PTY:s need special treatment, as shutdown doesn't work.
       (type . "enum io_type")
       
       ; For debugging purposes
       (label . "const char *")
       
       ; Used for raising i/o-exceptions.
       (e object exception_handler)
       
       ; User's close callback
       (close_callback object lsh_callback)

       (want_read . int)
       ; Called if poll indicates that data can be read. 
       (read object io_callback)

       (want_write . int)
       ; Called if poll indicates that data can be written.
       (write object io_callback)

       ; NOTE: We could put write_buffer inside the write callback, 
       ; but it seems simpler to keep it here, as it must be taken into
       ; account for proper closing of fd:s.
       
       (write_buffer object write_buffer)))
*/

#define FD_READ(fd) IO_CALLBACK((fd)->read, (fd))
#define FD_WRITE(fd) IO_CALLBACK((fd)->write, (fd))

/* Returns the number of open files. */
unsigned
io_nfiles(void);

/* Used for read handlers like read_line and read_packet that
 * processes a little data at a time, possibly replacing the handler
 * and leaving some data for the new one. */

/* GABA:
   (class
     (name io_buffered_read)
     (super io_callback)
     (vars
       (buffer_size . uint32_t)
       (handler object read_handler)))
*/

struct io_callback *
make_buffered_read(uint32_t buffer_size,
		   struct read_handler *handler);

/* Used for read handlers like read_data, that know how much data they
 * can consume. */

/* GABA:
   (class
     (name io_consuming_read)
     (super io_callback)
     (vars
       (query method uint32_t)
       ; Returns the maximum number of octets that
       ; can be consumed immediately.
       (consumer object abstract_write)))
*/

#define READ_QUERY(r) ((r)->query((r)))

void init_consuming_read(struct io_consuming_read *self,
			 struct abstract_write *consumer);

/* Passed to the listen callback, and to other functions and commands
 * dealing with addresses. */
/* GABA:
   (class
     (name address_info)
     (vars
       ; An ipnumber, in decimal dot notation, ipv6 format, or
       ; a dns name.
       (ip string)
       ; The port number here is always in host byte order
       (port . uint32_t))) */

/* Used for listening and connecting to local sockets.
 * Both strings have to be NUL-terminated. */

/* GABA:
   (class
     (name local_info)
     (vars
       (directory string)
       (name string)))
*/

struct local_info *
make_local_info(struct lsh_string *directory,
		struct lsh_string *name);
     
/* Returned by listen. And also by connect, so this is an improper name.
 * Functions related to AF_UNIX sockets leave the peer field as NULL. */
/* GABA:
   (class
     (name listen_value)
     (vars
       (fd object lsh_fd)
       (peer object address_info)
       (local object address_info)))
*/

struct listen_value *
make_listen_value(struct lsh_fd *fd,
		  struct address_info *peer,
		  struct address_info *local);

/* I/O exceptions */
/* GABA:
   (class
     (name io_exception)
     (super exception)
     (vars
       ; NULL if no fd was involved
       (fd object lsh_fd)
       ; errno code, or zero if not available
       (error . int))))
*/

/* If msg is NULL, it is derived from errno */
struct exception *
make_io_exception(uint32_t type, struct lsh_fd *fd, int error, const char *msg);

/* Used in cases where the fd and errno are not available */
#define STATIC_IO_EXCEPTION(type, name) \
{ { STATIC_HEADER, (type), (name) }, NULL, 0}

extern const struct exception finish_read_exception;
extern const struct exception finish_io_exception;

void
io_init(void);

void
io_final(void);

void io_run(void);

void
lsh_oop_register_read_fd(struct lsh_fd *fd);

void
lsh_oop_cancel_read_fd(struct lsh_fd *fd);

void
lsh_oop_register_write_fd(struct lsh_fd *fd);

void
lsh_oop_cancel_write_fd(struct lsh_fd *fd);

struct resource *
io_signal_handler(int signum,
		  struct lsh_callback *action);

struct resource *
io_callout(struct lsh_callback *action, unsigned seconds);

int blocking_read(int fd, struct read_handler *r);

int get_portno(const char *service, const char *protocol);

struct address_info *
make_address_info(struct lsh_string *host, 
		  uint32_t port);

struct address_info *
fd2info(struct lsh_fd *fd, int side);

struct address_info *
sockaddr2info(size_t addr_len,
	      struct sockaddr *addr);

struct sockaddr *
address_info2sockaddr(socklen_t *length,
		      struct address_info *a,
		      const int *preference,
		      int lookup);

unsigned
io_resolv_address(const char *host, const char *service,
		  unsigned default_port,
		  struct addr_queue *q);

/* Returns an exception, if anything went wrong */
const struct exception *
write_raw(int fd, uint32_t length, const uint8_t *data);

const struct exception *
read_raw(int fd, uint32_t length, uint8_t *data);

struct lsh_string *
io_read_file_raw(int fd, uint32_t guess);

void io_set_nonblocking(int fd);
void io_set_blocking(int fd);
void io_set_close_on_exec(int fd);
void io_init_fd(int fd, int shared);

struct lsh_fd *
make_lsh_fd(int fd, enum io_type type, const char *label,
	    struct exception_handler *e);

struct exception_handler *
make_exc_finish_read_handler(struct lsh_fd *fd,
			     struct exception_handler *parent,
			     const char *context);

/* GABA:
   (class
     (name connect_list_state)
     (super resource)
     (vars
       (q struct addr_queue)
       ;; Number of currently active fd:s
       (nfds . unsigned)
       (fds array (object lsh_fd) CONNECT_ATTEMPTS_LIMIT)))
*/

struct connect_list_state *
make_connect_list_state(void);

struct resource *
io_connect_list(struct connect_list_state *remote,
		struct command_continuation *c,
		struct exception_handler *e);

/* FIXME: Reorder arguments to put length first, for consistency? */
struct lsh_fd *
io_connect(struct sockaddr *remote,
	   socklen_t remote_length,
	   struct io_callback *c,
	   struct exception_handler *e);

struct io_callback *
make_connect_callback(struct command_continuation *c);

struct lsh_fd *
io_bind_sockaddr(struct sockaddr *local,
		 socklen_t length,
		 struct exception_handler *e);

struct lsh_fd *
io_listen(struct lsh_fd *fd,
	  struct io_callback *callback);

struct resource *
io_listen_list(struct addr_queue *addresses,
	       struct io_callback *callback,
	       struct exception_handler *e);

struct lsh_fd *
io_bind_local(struct local_info *info,
	      struct exception_handler *e);

struct lsh_fd *
io_connect_local(struct local_info *info,
		 struct command_continuation *c,
		 struct exception_handler *e);

struct io_callback *
make_listen_callback(struct command *c,
		     struct exception_handler *e);

struct lsh_fd *io_read_write(struct lsh_fd *fd,
			     struct io_callback *read,
			     uint32_t block_size,
			     struct lsh_callback *close_callback);

struct lsh_fd *io_read(struct lsh_fd *fd,
		       struct io_callback *read,
		       struct lsh_callback *close_callback);

struct lsh_fd *io_write(struct lsh_fd *fd,
			uint32_t block_size,
			struct lsh_callback *close_callback);

/* Close the fd right away. */
void close_fd(struct lsh_fd *fd);

/* Stop reading from the fd, and close it as soon as the buffer
 * is completely written. */
void close_fd_nicely(struct lsh_fd *fd);

/* Stop reading, but if the fd has a write callback, keep it open. */
void
close_fd_read(struct lsh_fd *fd);

void
close_fd_write(struct lsh_fd *fd);

struct abstract_write *
make_io_write_file(int fd, struct exception_handler *e);

struct abstract_write *
io_write_file(const char *fname, int flags,
	      int mode,
	      struct exception_handler *e);

int
lsh_make_pipe(int *fds);

int
lsh_popen(const char *program, const char **argv, int in,
	  pid_t *child);

struct lsh_string *
lsh_popen_read(const char *program, const char **argv, int in,
	       unsigned guess);

int
lsh_copy_file(int src, int dst);

/* Temporarily changing the current directory. */

int
lsh_pushd_fd(int dir);

int
lsh_pushd(const char *directory,
	  int *fd,
	  int create, int secret);
void
lsh_popd(int old_cd, const char *directory);


/* Socket workaround */
#ifndef SHUTDOWN_WORKS_WITH_UNIX_SOCKETS

/* There's an how++ missing in the af_unix shutdown implementation of
 * some linux versions. Try an ugly workaround. */
#ifdef linux

/* From src/linux/include/net/sock.h */
#define RCV_SHUTDOWN	1
#define SEND_SHUTDOWN	2

#define SHUT_RD_UNIX RCV_SHUTDOWN
#define SHUT_WR_UNIX SEND_SHUTDOWN
#define SHUT_RD_WR_UNIX (RCV_SHUTDOWN | SEND_SHUTDOWN)

#else /* !linux */

/* Don't know how to work around the broken shutdown. So disable it
 * completely. */

#define SHUTDOWN_UNIX(fd, how) 0

#endif /* !linux */
#endif /* !SHUTDOWN_WORKS_WITH_UNIX_SOCKETS */

#ifndef SHUTDOWN_UNIX
#define SHUTDOWN_UNIX(fd, how) (shutdown((fd), (how)))
#endif

#ifndef SHUT_RD
#define SHUT_RD 0
#endif

#ifndef SHUT_WR
#define SHUT_WR 1
#endif

#ifndef SHUT_RD_WR
#define SHUT_RD_WR 2
#endif

#ifndef SHUT_RD_UNIX
#define SHUT_RD_UNIX SHUT_RD
#endif

#ifndef SHUT_WR_UNIX
#define SHUT_WR_UNIX SHUT_WR
#endif

#ifndef SHUT_RD_WR_UNIX
#define SHUT_RD_WR_UNIX SHUT_RD_WR
#endif

#endif /* LSH_IO_H_INCLUDED */
