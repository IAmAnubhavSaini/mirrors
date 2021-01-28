/* io.c
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
#include <errno.h>
#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_POLL
# if HAVE_POLL_H
#  include <poll.h>
# elif HAVE_SYS_POLL_H
#  include <sys/poll.h>
# endif
#else
# include "jpoll.h"
#endif

#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
/* For the popen code */
#include <sys/wait.h>

#include <arpa/inet.h>

#include <oop.h>

#include "io.h"

#include "command.h"
#include "format.h"
#include "lsh_string.h"
#include "string_buffer.h"
#include "werror.h"
#include "xalloc.h"

/* Workaround for some version of FreeBSD. */
#ifdef POLLRDNORM
# define MY_POLLIN (POLLIN | POLLRDNORM)
#else /* !POLLRDNORM */
# define MY_POLLIN POLLIN
#endif /* !POLLRDNORM */

#define GABA_DEFINE
#include "io.h.x"
#undef GABA_DEFINE

#include "io.c.x"

/* Glue to liboop */

#define WITH_LIBOOP_SIGNAL_ADAPTER 1

/* Because of signal handlers, there can be only one oop object. */
static oop_source_sys *global_oop_sys = NULL;
#if WITH_LIBOOP_SIGNAL_ADAPTER
static oop_adapter_signal *global_oop_signal = NULL;
#endif
static oop_source *source = NULL;
static unsigned nfiles = 0;

/* OOP Callbacks */
static void *
lsh_oop_signal_callback(oop_source *s UNUSED, int sig, void *data)
{
  CAST(lsh_signal_handler, self, (struct lsh_object *) data);

  trace("lsh_oop_signal_callback: Signal %i, handler: %t\n",
	sig, self->action);
  
  assert(sig == self->signum);
  
  LSH_CALLBACK(self->action);

  return OOP_CONTINUE;
}

static void
lsh_oop_register_signal(struct lsh_signal_handler *handler)
{
  trace("lsh_oop_register_signal: signal: %i, handler: %t\n",
	handler->signum, handler);
  
  assert(source);
  if (handler->super.alive)
    source->on_signal(source, handler->signum,
		      lsh_oop_signal_callback, handler);
}

static void
lsh_oop_cancel_signal(struct lsh_signal_handler *handler)
{
  trace("lsh_oop_cancel_signal: signal: %i, handler: %t\n",
	handler->signum, handler);

  assert(source);
  if (handler->super.alive)
    source->cancel_signal(source, handler->signum,
			  lsh_oop_signal_callback, handler);
}

static void *
lsh_oop_fd_read_callback(oop_source *s UNUSED, int fileno,
			 oop_event event, void *data)
{
  CAST(lsh_fd, fd, (struct lsh_object *) data);

  assert(fileno == fd->fd);
  assert(event == OOP_READ);
  assert(fd->super.alive);

  trace("lsh_oop_fd_read_callback: fd %i: %z\n",
	fd->fd, fd->label);

  FD_READ(fd);

  return OOP_CONTINUE;
}

void
lsh_oop_register_read_fd(struct lsh_fd *fd)
{
  trace("lsh_oop_register_read_fd: fd: %i, %z\n",
	fd->fd, fd->label);
  
  assert(source);
  if (fd->super.alive && !fd->want_read)
    {
      assert(fd->read);
      
      source->on_fd(source, fd->fd, OOP_READ, lsh_oop_fd_read_callback, fd);
      fd->want_read = 1;
    }
}

void
lsh_oop_cancel_read_fd(struct lsh_fd *fd)
{
  trace("lsh_oop_cancel_read_fd: fd: %i, %z\n",
	fd->fd, fd->label);
  
  assert(source);
  if (fd->super.alive)
    {
      source->cancel_fd(source, fd->fd, OOP_READ);
      fd->want_read = 0;
    }
}

static void *
lsh_oop_fd_write_callback(oop_source *s UNUSED, int fileno,
			  oop_event event, void *data)
{
  CAST(lsh_fd, fd, (struct lsh_object *) data);

  assert(fileno == fd->fd);
  assert(event == OOP_WRITE);
  assert(fd->super.alive);
  
  trace("lsh_oop_fd_write_callback: fd %i: %z\n",
	fd->fd, fd->label);

  FD_WRITE(fd);

  return OOP_CONTINUE;
}

void
lsh_oop_register_write_fd(struct lsh_fd *fd)
{
  trace("lsh_oop_register_write_fd: fd: %i, %z\n",
	fd->fd, fd->label);
  
  assert(source);
  if (fd->super.alive && !fd->want_write)
    {
      assert(fd->write);
      
      source->on_fd(source, fd->fd, OOP_WRITE, lsh_oop_fd_write_callback, fd);
      fd->want_write = 1;
    }
}

void
lsh_oop_cancel_write_fd(struct lsh_fd *fd)
{
  trace("lsh_oop_cancel_write_fd: fd: %i, %z\n",
	fd->fd, fd->label);

  assert(source);
  if (fd->super.alive)
    {
      source->cancel_fd(source, fd->fd, OOP_WRITE);
      fd->want_write = 0;
    }
}

static void *
lsh_oop_time_callback(oop_source *source UNUSED,
                      struct timeval time UNUSED, void *data)
{
  CAST(lsh_callout, callout, (struct lsh_object *) data);

  assert(callout->super.alive);

  trace("lsh_oop_time_callback: action: %t\n",
        callout->action);
  
  callout->super.alive = 0;

  LSH_CALLBACK(callout->action);

  return OOP_CONTINUE;
}

static void
lsh_oop_register_callout(struct lsh_callout *callout)
{
  assert(source);
  trace("lsh_oop_register_callout: action: %t\n",
        callout->action);

  if (callout->super.alive)
    source->on_time(source, callout->when, lsh_oop_time_callback, callout);
}

static void
lsh_oop_cancel_callout(struct lsh_callout *callout)
{
  assert(source);
  trace("lsh_oop_cancel_callout: action: %t\n",
        callout->action);
  if (callout->super.alive)
    source->cancel_time(source, callout->when, lsh_oop_time_callback, callout);
}

static void *
lsh_oop_stop_callback(oop_source *source UNUSED,
                      struct timeval time UNUSED, void *data UNUSED)
{
  trace("lsh_oop_stop_callback\n");
  
  if (!nfiles)
    /* An arbitrary non-NULL value stops oop_sys_run. */
    return OOP_HALT;
  else
    return OOP_CONTINUE;
}

static void
lsh_oop_stop(void)
{
  assert(source);
  trace("lsh_oop_stop\n");
  source->on_time(source, OOP_TIME_NOW, lsh_oop_stop_callback, NULL);
}

static void
lsh_oop_cancel_stop(void)
{
  assert(source);
  trace("lsh_oop_cancel_stop\n");
  source->cancel_time(source, OOP_TIME_NOW, lsh_oop_stop_callback, NULL);
}

/* For debugging */
#if 0
static void
list_files(void)
{
  struct lsh_object *o;

  for (o = gc_iterate_objects(&lsh_fd_class, NULL); o; o = gc_iterate_objects(&lsh_fd_class, o))
    {
      CAST(lsh_fd, fd, o);

      werror("%z fd %i: %z, %xi\n", fd->super.alive ? "live" : "dead",
	     fd->fd, fd->label, (uint32_t) o);
    }
}
#endif

void
io_init(void)
{
  struct sigaction pipe;
  memset(&pipe, 0, sizeof(pipe));

  pipe.sa_handler = SIG_IGN;
  sigemptyset(&pipe.sa_mask);
  pipe.sa_flags = 0;
  
  if (sigaction(SIGPIPE, &pipe, NULL) < 0)
    fatal("Failed to ignore SIGPIPE.\n");

  assert(!global_oop_sys);
  global_oop_sys = oop_sys_new();
  if (!global_oop_sys)
    fatal("Failed to initialize liboop oop_sys.\n");

#if WITH_LIBOOP_SIGNAL_ADAPTER
  global_oop_signal = oop_signal_new(oop_sys_source(global_oop_sys));
  if (!global_oop_signal)
    fatal("Failed to initialize liboop oop_signal.\n");
  source = oop_signal_source(global_oop_signal);
#else
   source = oop_sys_source(global_oop_sys);
#endif
}

void
io_final(void)
{
  assert(source);
  gc_final();

  /* The final gc may have closed some files, and called lsh_oop_stop.
   * So we must unregister that before deleting the oop source. */
  lsh_oop_cancel_stop();

  /* There mustn't be any outstanding callbacks left. */
  assert(nfiles == 0);
  
#if WITH_LIBOOP_SIGNAL_ADAPTER
  oop_signal_delete(global_oop_signal);
  global_oop_signal = NULL;
#endif
  oop_sys_delete(global_oop_sys);
  global_oop_sys = NULL;
  source = NULL;
}

void
io_run(void)
{
  void *res = oop_sys_run(global_oop_sys);

  /* We need liboop-0.8, OOP_ERROR is not defined in liboop-0.7. */

  if (res == OOP_ERROR)
    werror("oop_sys_run %e\n", errno);

  trace("io_run: Exiting\n");
}


/* Calls trigged by a signal handler. */
/* GABA:
   (class
     (name lsh_signal_handler)
     (super resource)
     (vars
       (signum . int)
       (action object lsh_callback)))
*/

/* GABA:
   (class
     (name lsh_callout)
     (super resource)
     (vars
       (when . "struct timeval")
       (action object lsh_callback)))
*/


static void
do_kill_signal_handler(struct resource *s)
{
  CAST(lsh_signal_handler, self, s);

  if (self->super.alive)
    {
      lsh_oop_cancel_signal(self);
      self->super.alive = 0;
    }
}

struct resource *
io_signal_handler(int signum,
		  struct lsh_callback *action)
{
  NEW(lsh_signal_handler, handler);

  init_resource(&handler->super, do_kill_signal_handler);

  handler->signum = signum;
  handler->action = action;

  lsh_oop_register_signal(handler);
  gc_global(&handler->super);
  
  return &handler->super;
}

static void
do_kill_callout(struct resource *s)
{
  CAST(lsh_callout, self, s);

  if (self->super.alive)
    {
      lsh_oop_cancel_callout(self);
      self->super.alive = 0;
    }
}

struct resource *
io_callout(struct lsh_callback *action, unsigned seconds)
{
  NEW(lsh_callout, self);
  init_resource(&self->super, do_kill_callout);

  if (seconds)
    {
      /* NOTE: Using absolute times, like oop does, is a little
       * dangerous if the system time is changed abruptly. */
      if (gettimeofday(&self->when, NULL) < 0)
	fatal("io_callout: gettimeofday failed!\n");
      self->when.tv_sec += seconds;
    }
  else
    self->when = OOP_TIME_NOW;
  
  self->action = action;
      
  lsh_oop_register_callout(self);
  
  gc_global(&self->super);
  return &self->super;
}

/* Read-related callbacks */

static void
do_buffered_read(struct io_callback *s,
		 struct lsh_fd *fd)
{
  CAST(io_buffered_read, self, s);
  uint8_t *buffer = alloca(self->buffer_size);
  int res;

  assert(fd->want_read);   

  /* FIXME: Use a string and lsh_string_read instead? */
  res = read(fd->fd, buffer, self->buffer_size);
  
  if (res < 0)
    switch(errno)
      {
      case EINTR:
	break;
      case EWOULDBLOCK:
	werror("io.c: read_callback: Unexpected EWOULDBLOCK\n");
	break;
      case EPIPE:
	/* Getting EPIPE from read seems strange, but appearantly
	 * it happens sometimes. */
	werror("Unexpected EPIPE from read.\n");
      default:
	EXCEPTION_RAISE(fd->e, 
			make_io_exception(EXC_IO_READ, fd,
					  errno, NULL));
	/* Close the fd, unless it has a write callback. */
	close_fd_read(fd);
	
	break;
      }
  else if (res > 0)
    {
      uint32_t left = res;
    
      while (fd->super.alive && fd->read && left)
	{
	  uint32_t done;

	  /* NOTE: What to do if want_read is false? To improve the
	   * connection_lock mechanism, it must be possible to
	   * temporarily stop reading, which means that fd->want_read
	   * has to be cleared.
	   *
	   * But when doing this, we have to keep the data that we
	   * have read, some of which is buffered here, on the stack,
	   * and the rest inside the read-handler.
	   *
	   * There are two alternatives: Save our buffer here, or
	   * continue looping, letting the read-handler process it
	   * into packets. In the latter case, the ssh_connection
	   * could keep a queue of waiting packets, but it would still
	   * have to clear the want_read flag, to prevent that queue
	   * from growing arbitrarily large.
	   *
	   * We now go with the second alternative. */

	  assert(self->handler);

	  /* NOTE: This call may replace self->handler */
	  done = READ_HANDLER(self->handler, left, buffer);
	  
	  buffer += done;
	  left -= done;

	  if (!fd->want_read)
	    debug("do_buffered_read: want_read = 0; handler needs a pause.\n");
	  
	  if (fd->want_read && !self->handler)
	    {
	      werror("do_buffered_read: Handler disappeared! Ignoring %i bytes\n",
		     left);
	      lsh_oop_cancel_read_fd(fd);
	      return;
	    }
	}

      if (left)
	verbose("read_buffered: fd died, %i buffered bytes discarded\n",
		left);
    }
  else
    {
      /* We have read EOF. Pass available == 0 to the handler */
      assert(fd->super.alive);
      assert(fd->read);
      assert(fd->want_read);
      assert(self->handler);

      trace("io.c: do_buffered_read: EOF on fd %i: %z\n",
	    fd->fd, fd->label);

      /* Close the fd, unless it has a write callback. */
      close_fd_read(fd);
      
      READ_HANDLER(self->handler, 0, NULL);
    }
	
}

struct io_callback *
make_buffered_read(uint32_t buffer_size,
		   struct read_handler *handler)
{
  NEW(io_buffered_read, self);

  self->super.f = do_buffered_read;
  self->buffer_size = buffer_size;
  self->handler = handler;

  return &self->super;
}

static void
do_consuming_read(struct io_callback *c,
		  struct lsh_fd *fd)
{
  CAST_SUBTYPE(io_consuming_read, self, c);
  uint32_t wanted = READ_QUERY(self);

  assert(fd->want_read);
  
  if (!wanted)
    {
      lsh_oop_cancel_read_fd(fd);
    }
  else
    {
      struct lsh_string *s = lsh_string_alloc(wanted);
      int res = lsh_string_read(s, 0, fd->fd, wanted);

      if (res < 0)
	{
	  switch(errno)
	    {
	    case EINTR:
	      break;
	    case EWOULDBLOCK:
	      werror("io.c: read_consume: Unexpected EWOULDBLOCK\n");
	      break;
	    case EPIPE:
	      /* FIXME: I don't understand why reading should return
	       * EPIPE, but it happens occasionally under linux. Perhaps
	       * we should treat it as EOF instead? */
	      werror("io.c: read_consume: Unexpected EPIPE.\n");
	      /* Fall through */
	    default:
	      EXCEPTION_RAISE(fd->e, 
			      make_io_exception(EXC_IO_READ,
						fd, errno, NULL));
	      break;
	    }
	  lsh_string_free(s);
	}
      else if (res > 0)
	{
	  lsh_string_trunc(s, res);
	  A_WRITE(self->consumer, s);
	}
      else
	{
	  trace("io.c: do_consuming_read: EOF on fd %i: %z\n",
		fd->fd, fd->label);

	  lsh_string_free(s);

	  /* Close the fd, unless it has a write callback. */
	  A_WRITE(self->consumer, NULL);
	  close_fd_read(fd);
	}
    }
}

/* NOTE: Doesn't initialize the query field. That should be done in
 * the subclass's constructor. */
void init_consuming_read(struct io_consuming_read *self,
			 struct abstract_write *consumer)
{
  self->super.f = do_consuming_read;
  self->consumer = consumer;
}


/* Write related callbacks */
static void
do_write_callback(struct io_callback *s UNUSED,
		  struct lsh_fd *fd)
{
  /* CAST(io_write_callback, self, s); */
  assert(fd->super.alive);
  
  if (!write_buffer_pre_write(fd->write_buffer))
    {
      /* Buffer is empty */
      if (fd->write_buffer->closed)
        close_fd_write(fd);
      else
	lsh_oop_cancel_write_fd(fd);
    }
  else
    {
      uint32_t size;
      int res;

      size = MIN(fd->write_buffer->end - fd->write_buffer->start,
		 fd->write_buffer->block_size);
      assert(size);
  
      res = write(fd->fd,
		  lsh_string_data(fd->write_buffer->buffer) + fd->write_buffer->start,
		  size);
      if (!res)
	fatal("Closed?");
      if (res < 0)
	switch(errno)
	  {
	  case EINTR:
	  case EAGAIN:
	    break;
	  case EPIPE:
	    debug("io.c: Broken pipe.\n");

	    /* Fall through */
	  default:
#if 0
	    /* Don't complain if it was writing the final ^D
	     * character that failed. */
	    if ( (fd->type == IO_PTY_CLOSED)
		 && (fd->write_buffer->length == 1) )
	      debug("io.c: ignoring write error, on the final ^D character\n");
#endif
	    werror("io.c: write failed %e\n", errno);
	    EXCEPTION_RAISE(fd->e,
			    make_io_exception(EXC_IO_WRITE,
					      fd, errno, NULL));
	    close_fd(fd);
	    
	    break;
	  }
      else
	write_buffer_consume(fd->write_buffer, res);
    }
}

static struct io_callback io_write_callback =
{ STATIC_HEADER, do_write_callback };


struct listen_value *
make_listen_value(struct lsh_fd *fd,
		  struct address_info *peer,
		  struct address_info *local)
{
  NEW(listen_value, self);

  self->fd = fd;
  self->peer = peer;
  self->local = local;

  return self;
}


/* Listen callback */

/* GABA:
   (class
     (name io_listen_callback)
     (super io_callback)
     (vars
       (c object command)
       ;; FIXME: Is this needed? We could just inherit
       ;; the listening fd's exception handler.
       (e object exception_handler)))
*/

static void
do_listen_callback(struct io_callback *s,
		   struct lsh_fd *fd)
{
  CAST(io_listen_callback, self, s);

#if WITH_IPV6
  struct sockaddr_storage peer;
#else
  struct sockaddr_in peer;
#endif

  socklen_t addr_len = sizeof(peer);
  int conn;

  conn = accept(fd->fd,
		(struct sockaddr *) &peer, &addr_len);
  if (conn < 0)
    {
      werror("io.c: accept failed %e", errno);
      return;
    }

  trace("io.c: accept on fd %i\n", conn);
  COMMAND_CALL(self->c,
	       make_listen_value(make_lsh_fd(conn, IO_NORMAL,
					     "accepted socket", self->e),
				 sockaddr2info(addr_len,
					       (struct sockaddr *) &peer), 
				 fd2info(fd,0)),
	       &discard_continuation, self->e);
}

struct io_callback *
make_listen_callback(struct command *c,
		     struct exception_handler *e)
{
  NEW(io_listen_callback, self);
  self->super.f = do_listen_callback;
  self->c = c;
  self->e = e;
  
  return &self->super;
}


/* Connect callback */

/* GABA:
   (class
     (name io_connect_callback)
     (super io_callback)
     (vars
       (c object command_continuation)))
*/

static void
do_connect_callback(struct io_callback *s,
		    struct lsh_fd *fd)
{
  CAST(io_connect_callback, self, s);
  int socket_error;
  socklen_t len = sizeof(socket_error);
  
  /* Check if the connection was successful */
  if ((getsockopt(fd->fd, SOL_SOCKET, SO_ERROR,
		  (char *) &socket_error, &len) < 0)
      || socket_error)
    {
      trace("io.c: connect_callback: Connect on fd %i failed.\n", fd->fd);
      EXCEPTION_RAISE(fd->e,
		      make_io_exception(EXC_IO_CONNECT, fd,
					0, "Connect failed."));
      close_fd(fd);
    }
  else
    {
      trace("io.c: connect_callback: fd %i connected.\n", fd->fd);
      fd->write = NULL;
      lsh_oop_cancel_write_fd(fd);
      fd->label = "connected socket";
      COMMAND_RETURN(self->c, fd);
    }
}

struct io_callback *
make_connect_callback(struct command_continuation *c)
{
  NEW(io_connect_callback, self);

  self->super.f = do_connect_callback;
  self->c = c;

  return &self->super;
}


/* This function is called if a connection this file somehow depends
 * on disappears. For instance, the connection may have spawned a
 * child process, and this file may be the stdin of that process. */

/* To kill a file, mark it for closing and the backend will do the work. */
static void do_kill_fd(struct resource *r)
{
  CAST_SUBTYPE(lsh_fd, fd, r);

  /* We use close_fd_nicely, so that any data in the write buffer is
   * flushed before the fd is closed. */
  if (r->alive)
    close_fd_nicely(fd);
}

/* Closes the file on i/o errors, and passes the exception on */

static void
do_exc_io_handler(struct exception_handler *self,
		  const struct exception *x)
{
  if (x->type & EXC_IO)
    {
      CAST_SUBTYPE(io_exception, e, x);

      if (e->fd)
	close_fd(e->fd);
    }
  EXCEPTION_RAISE(self->parent, x);
  return;
}


/* These functions are used by werror and friends */

/* For fd:s in blocking mode. */
const struct exception *
write_raw(int fd, uint32_t length, const uint8_t *data)
{
  while(length)
    {
      int written = write(fd, data, length);

      if (written < 0)
	switch(errno)
	  {
	  case EINTR:
	  case EAGAIN:
	    continue;
	  default:
	    return make_io_exception(EXC_IO_BLOCKING_WRITE,
				     NULL, errno, NULL);
	  }
      
      length -= written;
      data += written;
    }
  return NULL;
}

/* For fd:s in blocking mode. */
const struct exception *
read_raw(int fd, uint32_t length, uint8_t *data)
{
  while(length)
    {
      int done = read(fd, data, length);

      if (done < 0)
	switch(errno)
	  {
	  case EINTR:
	  case EAGAIN:
	    continue;
	  default:
	    return make_io_exception(EXC_IO_BLOCKING_READ,
				     NULL, errno, NULL);
	  }
      else if (done == 0)
	{
	  /* EOF. */
	  /* FIXME: Indicate the amount of data read, somehow. */
	  return make_io_exception(EXC_IO_BLOCKING_READ,
				   NULL, 0, NULL);
	}
	
      length -= done;
      data += done;
    }
  return NULL;
}

struct lsh_string *
io_read_file_raw(int fd, uint32_t guess)
{
  struct string_buffer buffer;
  string_buffer_init(&buffer, guess);

  for (;;)
    {
      int res;
      
      if (!buffer.left)
        /* Roughly double the size of the buffer */
        string_buffer_grow(&buffer,
                           lsh_string_length(buffer.partial) + buffer.total + 100);

      res = lsh_string_read(buffer.partial, buffer.pos,
			    fd, buffer.left);
      
      if (res < 0)
        {
          if (errno == EINTR)
            continue;
          
          string_buffer_clear(&buffer);
          return NULL;
        }
      else if (!res)
        {
          /* EOF */
          return string_buffer_final(&buffer, buffer.left);
        }
      assert( (unsigned) res <= buffer.left);
      
      buffer.pos += res;
      buffer.left -= res;
    }
}

/* Network utility functions */

/* Converts a string port number or service name to a port number.
 * Returns the port number in _host_ byte order, or 0 if lookup
 * fails. */

int get_portno(const char *service, const char *protocol)
{
  if (service == NULL)
    return 0;
  else
    {
      char *end;
      long portno;

      portno = strtol(service, &end, 10);
      if (portno > 0
	  &&  portno <= 65535
	  &&  end != service
	  &&  *end == '\0')
	  return portno;
      else
	{
	  struct servent * serv;

	  serv = getservbyname(service, protocol);
	  if (!serv)
	    return 0;
	  return ntohs(serv->s_port);
	}
    }
}

struct address_info *
make_address_info(struct lsh_string *host, uint32_t port)
{
  NEW(address_info, info);

  info->port = port;
  info->ip = host;
  return info;
}

struct address_info *
sockaddr2info(size_t addr_len,
	      struct sockaddr *addr)
{
  NEW(address_info, info);
  
  switch(addr->sa_family)
    {
    case AF_INET:
      assert(addr_len == sizeof(struct sockaddr_in));
      {
	struct sockaddr_in *in = (struct sockaddr_in *) addr;
	uint32_t ip = ntohl(in->sin_addr.s_addr);
	
	info->port = ntohs(in->sin_port);
	info->ip = ssh_format("%di.%di.%di.%di",
			      (ip >> 24) & 0xff,
			      (ip >> 16) & 0xff,
			      (ip >> 8) & 0xff,
			      ip & 0xff);
      }
      return info;
      
#if WITH_IPV6
    case AF_INET6:
      assert(addr_len == sizeof(struct sockaddr_in6));
      {
	struct sockaddr_in6 *in = (struct sockaddr_in6 *) addr;

	info->port = ntohs(in->sin6_port);
	info->ip = lsh_string_ntop(addr->sa_family, INET6_ADDRSTRLEN,
				   &in->sin6_addr);

      }
      return info;
#endif /* WITH_IPV6 */

    case AF_UNIX:
      /* Silently return NULL. This happens when a gateway client
       * connects. */
      return NULL;
    default:
      werror("io.c: sockaddr2info: Unsupported address family.\n");
      return NULL;
    }
}

struct address_info *
fd2info(struct lsh_fd *fd, int side)
{
#if WITH_IPV6
  struct sockaddr_storage sock;
#else
  struct sockaddr_in sock;
#endif
  
  socklen_t s_len = sizeof(sock);

  int get;

  if( !side ) /* Local */
    get = getsockname( fd->fd, (struct sockaddr *)  &sock, &s_len );
  else
    get = getpeername( fd->fd, (struct sockaddr *)  &sock, &s_len );
  
  if (get < 0)  
    {               
      werror("io.c: getXXXXname failed %e", errno);
      return NULL;
    }

  return sockaddr2info(s_len,
		       (struct sockaddr *) &sock);
}

#if HAVE_GETADDRINFO
static struct addrinfo *
choose_address(struct addrinfo *list,
	       const int *preference)
{
  int i;
  for (i = 0; preference[i]; i++)
    {
      struct addrinfo *p;
      for (p = list; p; p = p->ai_next)
	if (preference[i] == p->ai_family)
	  return p;
    }
  return NULL;
}
#endif /* HAVE_GETADDRINFO */

/* Uses NUL-terminated strings. HOST can be NULL, meaning any address.
 * If PORT is NULL or can't be looked up, and DEFAULT_PORT is non-zero,
 * the DEFAULT_PORT is used. */
unsigned
io_resolv_address(const char *host, const char *service,
		  unsigned default_port,
		  struct addr_queue *q)
{
  unsigned naddresses = 0;
  
  /* Some systems have getaddrinfo, but still doesn't implement all of
   * RFC 2553 */
#if defined(HAVE_GETADDRINFO) && \
    defined(HAVE_GAI_STRERROR) && defined(HAVE_AI_NUMERICHOST)
  {
    struct addrinfo hints;
    struct addrinfo *list;
    struct addrinfo *p;
    int err;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (!service || (err = getaddrinfo(host, service, &hints, &list)))
      {
	/* FIXME: It seems ugly to have to convert the port number to
	 * a string. Perhaps we can call getaddrinfo with a NULL or
	 * dummy service and patch in the right port number later? */
	
	struct lsh_string *port = ssh_format("%di", default_port);
	err = getaddrinfo(host, lsh_get_cstring(port), &hints, &list);
	lsh_string_free(port);
      }
    
    if (err)
      {
	debug("io_listen_address: getaddrinfo failed (err = %i): %z\n",
	      err, gai_strerror(err));
	return 0;
      }

    for (p = list; p; p = p->ai_next)
      {
	/* FIXME: Do we need to filter out some address families? */
	struct sockaddr *n = addr_queue_add_tail(q, p->ai_addrlen);
	memcpy(n, p->ai_addr, p->ai_addrlen);
	naddresses++;
      }
    
    freeaddrinfo(list);
  }
#else
  /* !(defined(HAVE_GETADDRINFO) &&
     defined(HAVE_GAI_STRERROR) && defined(HAVE_AI_NUMERICHOST) */ 

#if WITH_IPV6
#error IPv6 enabled, but getaddrinfo and friends were not found. 
#endif

  if (strchr(host, ':'))
    {
      debug("io_resolv_address: Literal IPv6 used. Failing.\n");
      return 0;
    }
  else
    {
      struct sockaddr_in addr;
      unsigned port;
      if (!service || !(port = get_portno(service, "tcp")))
	port = default_port;

      /* Needed to clear addr.sin_zero on systems that have that
       * field. In particular, AIX. */
      memset(&addr, 0, sizeof(addr));
      
      addr.sin_port = htons(port);

      /* Use IPv4 only */
      addr.sin_family = AF_INET;
    
      if (!host)
	/* Any interface */
	addr.sin_addr.s_addr = INADDR_ANY;

      else
	{
	  /* First check for numerical ip-number */
#if HAVE_INET_ATON
	  if (!inet_aton(host, &addr.sin_addr))
#else /* !HAVE_INET_ATON */
	    /* NOTE: It is wrong to work with ((unsigned long int) -1)
	     * directly, as this breaks Linux/Alpha systems. But
	     * INADDR_NONE isn't portable. That's what inet_aton is for;
	     * see the GNU libc documentation. */
# ifndef INADDR_NONE
# define INADDR_NONE ((unsigned long int) -1)
# endif /* !INADDR_NONE */
	  addr.sin_addr.s_addr = inet_addr(host);
	  if (addr.sin_addr.s_addr == INADDR_NONE)
#endif  /* !HAVE_INET_ATON */
	    {
	      struct hostent *hp;
	      unsigned i;
	      if (! ( (hp = gethostbyname(host))
		      && (hp->h_addrtype == AF_INET)))
		return 0;

	      assert(hp->h_length == sizeof(addr.sin_addr));
	      
	      for (i = 0; hp->h_addr_list[i]; i++)
		{
		  struct sockaddr *n;
		  memcpy(&addr.sin_addr, hp->h_addr_list[i], hp->h_length);
		  n = addr_queue_add_tail(q, sizeof(addr));
		  memcpy(n, &addr, sizeof(addr));
		  naddresses++;
		}
	    }
	}
    }
#endif /* !HAVE_GETADDRINFO */
    
  return naddresses;
}


/* FIXME: Perhaps this function should be changed to return a list of
 * sockaddr:s? */
struct sockaddr *
address_info2sockaddr(socklen_t *length,
		      struct address_info *a,
		      /* Preferred address families. Zero-terminated array. */
		      const int *preference,
		      int lookup)
{
  const char *host;

  if (a->ip)
    {
      host = lsh_get_cstring(a->ip);
      if (!host)
	{
	  debug("address_info2sockaddr: hostname contains NUL characters.\n");
	  return NULL;
	}
    }
  else
    host = NULL;

  /* Some systems have getaddrinfo, but still doesn't implement all of
   * RFC 2553 */
#if defined(HAVE_GETADDRINFO) && \
    defined(HAVE_GAI_STRERROR) && defined(HAVE_AI_NUMERICHOST)
  {
    struct addrinfo hints;
    struct addrinfo *list;
    struct addrinfo *chosen;
    struct sockaddr *res;
    const int default_preference
#if WITH_IPV6
      [3] = { AF_INET, AF_INET6, 0 }
#else
      [2] = { AF_INET, 0 }
#endif      
      ;
    int err;
    /* FIXME: It seems ugly to have to convert the port number to a
     * string. */
    struct lsh_string *service = ssh_format("%di", a->port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (!lookup)
      hints.ai_flags |= AI_NUMERICHOST;
    
    err = getaddrinfo(host, lsh_get_cstring(service), &hints, &list);
    lsh_string_free(service);

    if (err)
      {
	debug("address_info2sockaddr: getaddrinfo failed (err = %i): %z\n",
	      err, gai_strerror(err));
	return NULL;
      }

    chosen = choose_address(list,
			    preference ? preference : default_preference);
    if (!chosen)
      {
	freeaddrinfo(list);
	return NULL;
      }
    
    *length = chosen->ai_addrlen;
    
    res = lsh_space_alloc(*length);
    memcpy(res, chosen->ai_addr, *length);
    freeaddrinfo(list);

    return res;
  }
#else
/* !(defined(HAVE_GETADDRINFO) &&
     defined(HAVE_GAI_STRERROR) && defined(HAVE_AI_NUMERICHOST) */ 

#if WITH_IPV6
#error IPv6 enabled, but getaddrinfo and friends were not found. 
#endif

  if (host && strchr(host, ':'))
    {
      debug("address_info2sockaddr: Literal IPv6 used. Failing.\n");
      return NULL;
    }
  else
    {
      struct sockaddr_in *addr;
      NEW_SPACE(addr);

      *length = sizeof(*addr);
      addr->sin_port = htons(a->port);

      /* Use IPv4 only */
      addr->sin_family = AF_INET;
    
      if (!host)
	/* Any interface */
	addr->sin_addr.s_addr = INADDR_ANY;

      else
	{
	  /* First check for numerical ip-number */
#if HAVE_INET_ATON
	  if (!inet_aton(host, &addr->sin_addr))
#else /* !HAVE_INET_ATON */
	    /* NOTE: It is wrong to work with ((unsigned long int) -1)
	     * directly, as this breaks Linux/Alpha systems. But
	     * INADDR_NONE isn't portable. That's what inet_aton is for;
	     * see the GNU libc documentation. */
# ifndef INADDR_NONE
# define INADDR_NONE ((unsigned long int) -1)
# endif /* !INADDR_NONE */
	  addr->sin_addr.s_addr = inet_addr(host);
	  if (addr->sin_addr.s_addr == INADDR_NONE)
#endif  /* !HAVE_INET_ATON */
	    {
	      struct hostent *hp;

	      if (! (lookup 
		     && (hp = gethostbyname(host))
		     && (hp->h_addrtype == AF_INET)))
		{
		  lsh_space_free(addr);
		  return NULL;
		}

	      memcpy(&addr->sin_addr, hp->h_addr, hp->h_length);
	    }
	}
      return (struct sockaddr *) addr;
    }
#endif /* !HAVE_GETADDRINFO */  
}

static void
handle_nonblock_error(const char *msg)
{
  /* On BSD, trying to set /dev/null in nonblocking mode fails with
   * errno 19, ENODEV. We have to ignore that.
   *
   * And on FreeBSD 5, the error code is changed to ENOTTY, for the
   * same problem.
   *
   * For now, still display a warning message, to keep track of when
   * and where it occurs.
   */
  if (errno == ENODEV || errno == ENOTTY)
    werror("%z %e\n", msg, errno);
  else
    fatal("%z %e\n", msg, errno);
}

void io_set_nonblocking(int fd)
{
  int old = fcntl(fd, F_GETFL);

  if (old < 0)
    fatal("io_set_nonblocking: fcntl(F_GETFL) failed %e\n", errno);
  
  if (fcntl(fd, F_SETFL, old | O_NONBLOCK) < 0)
    handle_nonblock_error("io_set_nonblocking: fcntl(F_SETFL) failed");
}

void io_set_close_on_exec(int fd)
{
  /* NOTE: There's only one documented flag bit, so reading the old
   * value should be redundant. */
  
  int old = fcntl(fd, F_GETFD);

  if (old < 0)
    fatal("io_set_close_on_exec: fcntl(F_GETFD) failed %e\n", errno);
  
  if (fcntl(fd, F_SETFD, old | 1) < 0)
    fatal("Can't set close-on-exec flag for fd %i %e\n", fd, errno);
}


/* *All* file descripters handled by the backend should have the
   close-on-exec flag set, and all but shared stdio descriptors should
   use non-blocking mode. */

void io_init_fd(int fd, int shared)
{
  io_set_close_on_exec(fd);

  if (!shared)
    io_set_nonblocking(fd);
}

struct lsh_fd *
make_lsh_fd(int fd, enum io_type type, const char *label,
	    struct exception_handler *e)
{
  NEW(lsh_fd, self);

  nfiles++;
  /* NOTE: Relies on order of the enum constants. */
  io_init_fd(fd, type >= IO_STDIO);

  init_resource(&self->super, do_kill_fd);

  self->fd = fd;
  self->type = type;
  self->label = label;
  
  self->e = make_exception_handler(do_exc_io_handler, e, HANDLER_CONTEXT);
  
  self->close_callback = NULL;

  self->want_read = 0;
  self->read = NULL;

  self->want_write = 0;
  self->write = NULL;

  gc_global(&self->super);
  return self;
}

unsigned
io_nfiles(void)
{
  return nfiles;
}

/* Some code is taken from Thomas Bellman's tcputils. */
struct lsh_fd *
io_connect(struct sockaddr *remote,
	   socklen_t remote_length,
	   struct io_callback *c,
	   struct exception_handler *e)
{
  int s = socket(remote->sa_family, SOCK_STREAM, 0);
  struct lsh_fd *fd;
  
  if (s<0)
    return NULL;

  trace("io_connect: Connecting using fd %i\n", s);
  
  io_init_fd(s, 0);

#if 0
  if (local  &&  bind(s, (struct sockaddr *)local, sizeof *local) < 0)
    {
      int saved_errno = errno;
      close(s);
      errno = saved_errno;
      return NULL;
    }
#endif
  
  if ( (connect(s, remote, remote_length) < 0)
       && (errno != EINPROGRESS) )       
    {
      int saved_errno = errno;
      close(s);
      errno = saved_errno;
      return NULL;
    }

  fd = make_lsh_fd(s, IO_NORMAL, "connecting socket", e);
  
  fd->write = c;
  lsh_oop_register_write_fd(fd);
    
  return fd;
}

/* connect_list_state */
static void
do_connect_list_kill(struct resource *s)
{
  CAST(connect_list_state, self, s);

  if (self->super.alive)
    {
      unsigned i;
      self->super.alive = 0;
      for (i = 0; i < CONNECT_ATTEMPTS_LIMIT; i++)
	{
	  struct lsh_fd *fd = self->fds[i];
	  if (fd)
	    KILL_RESOURCE(&fd->super);
	}
    }
}

struct connect_list_state *
make_connect_list_state(void)
{
  NEW(connect_list_state, self);
  unsigned i;

  init_resource(&self->super, do_connect_list_kill);
  addr_queue_init(&self->q);
  self->nfds = 0;
  
  for (i = 0; i < CONNECT_ATTEMPTS_LIMIT; i++)
    self->fds[i] = NULL;

  return self;
}

/* GABA:
   (class
     (name connect_list_callback)
     (super io_callback)
     (vars
       (state object connect_list_state)
       (index . unsigned)
       (c object command_continuation)
       (e object exception_handler)))
*/

static void
connect_attempt(struct connect_list_callback *self)
{
  struct connect_list_state *state = self->state;
  struct lsh_fd * fd;
  socklen_t addr_length;
  struct sockaddr *addr;
  
  assert(state->super.alive);

  state->fds[self->index] = NULL;
  
  while ( (addr = addr_queue_peek_head(&state->q, &addr_length)))
    {
      fd = io_connect(addr, addr_length,
		      &self->super, self->e);
      addr_queue_remove_head(&state->q);

      if (fd)
	{
	  state->fds[self->index] = fd;
	  state->nfds++;
	  break;
	}
    }
}

static void
do_connect_list_callback(struct io_callback *s,
			 struct lsh_fd *fd)
{
  CAST(connect_list_callback, self, s);
  int socket_error;
  socklen_t len = sizeof(socket_error);
  
  assert(self->state->nfds);
  assert(self->index < CONNECT_ATTEMPTS_LIMIT);
  assert(fd == self->state->fds[self->index]);

  /* Check if the connection was successful */
  if ((getsockopt(fd->fd, SOL_SOCKET, SO_ERROR,
		  (char *) &socket_error, &len) < 0)
      || socket_error)
    {
      trace("io.c: connect_callback: Connect on fd %i failed.\n", fd->fd);
      close_fd(fd);
      self->state->nfds--;

      connect_attempt(self);
      if (!self->state->nfds)
	{
	  /* All addresses failed */
	  EXCEPTION_RAISE(self->e,
			  make_io_exception(EXC_IO_CONNECT, NULL,
					    0, "Connect failed"));
	  KILL_RESOURCE(&self->state->super);
	  return;
	}
    }
  else
    { /* Success */
      /* Remove the successful fd, and kill the rest */
      self->state->fds[self->index] = NULL;
      KILL_RESOURCE(&self->state->super);
      fd->label = "connected socket";
      COMMAND_RETURN(self->c, fd);
    }
}

static struct connect_list_callback *
make_connect_list_callback(struct connect_list_state *state,
			   unsigned index,
			   struct command_continuation *c,
			   struct exception_handler *e)
{
  NEW(connect_list_callback, self);

  self->super.f = do_connect_list_callback;
  self->state = state;
  self->index = index;
  self->c = c;
  self->e = e;
  
  return self;
}

/* Tries to connect to the addresses in the list, by attempting
 * CONNECT_ATTEMPTS_LIMIT connect calls in parallel. The first
 * successful connection (if any) is returned.
 *
 * Addresses are removed from the list as we go. */

/* FIXME: The returned value is never used */
struct resource *
io_connect_list(struct connect_list_state *state,
		struct command_continuation *c,
		struct exception_handler *e)
{
  unsigned i;

  for (i = 0;
       !addr_queue_is_empty(&state->q)
	 && i < CONNECT_ATTEMPTS_LIMIT;
       i++)
    connect_attempt(make_connect_list_callback(state, i, c, e));

  if (!state->nfds)
    {
      /* All addresses failed */
      EXCEPTION_RAISE(e,
		      make_io_exception(EXC_IO_CONNECT, NULL,
					0, "Connect failed"));
      KILL_RESOURCE(&state->super);
      return NULL;
    }
  
  return &state->super;
}


struct lsh_fd *
io_bind_sockaddr(struct sockaddr *local,
		 socklen_t length,
		 struct exception_handler *e)
{
  int s = socket(local->sa_family, SOCK_STREAM, 0);
  
  if (s<0)
    return NULL;

  trace("io.c: Trying to bind fd %i\n", s);

  {
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof yes);
#if WITH_IPV6 && defined (IPV6_V6ONLY)
    if (local->sa_family == AF_INET6)
      {
        if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes)) < 0)
          werror("setsockopt IPV6_V6ONLY failed: %e.\n", errno);
      }
#endif
  }

  if (bind(s, local, length) < 0)
    {
      trace("io.c: bind failed %e\n", errno);
      close(s);
      return NULL;
    }

  return make_lsh_fd(s, IO_NORMAL, "bound socket", e);
}

struct lsh_fd *
io_listen(struct lsh_fd *fd,
	  struct io_callback *callback)
{
  /* For convenience in nested function calls. */
  if (!fd)
    return NULL;
  
  if (listen(fd->fd, 256) < 0) 
    {
      /* Closes the fd and returns failure.
       * Caller is responsible for checking the return value and raising
       * a proper exception. */
      close_fd(fd);
      return NULL;
    }

  fd->read = callback;
  lsh_oop_register_read_fd(fd);

  return fd;
}

struct resource *
io_listen_list(struct addr_queue *addresses,
	       struct io_callback *callback,
	       struct exception_handler *e)
{
  struct resource_list *resources = make_resource_list();
  unsigned nbound = 0;
  socklen_t addr_length;
  struct sockaddr *addr;
  
  for (nbound = 0;
       (addr = addr_queue_peek_head(addresses, &addr_length));
       addr_queue_remove_head(addresses))
    {
      struct lsh_fd *fd;
      debug("listen_list: Trying to bind address of type %i\n",
	    addr->sa_family);

      fd = io_bind_sockaddr(addr, addr_length, e);
      if (fd)
	{
	  if (io_listen(fd, callback))
	    {
	      remember_resource(resources, &fd->super);
	      nbound++;
	    }
	  else
	    close_fd(fd);
	}
    }
  if (nbound)
    return &resources->super;
  else
    {
      KILL_RESOURCE_LIST(resources);
      return NULL;
    }
}


/* AF_LOCAL sockets */

/* Requires DIRECTORY and NAME to be NUL-terminated */

struct local_info *
make_local_info(struct lsh_string *directory,
		struct lsh_string *name)
{
  if (!directory || !name
      || memchr(lsh_string_data(name), '/', lsh_string_length(name)))
    return NULL;

  assert(lsh_get_cstring(directory));
  assert(lsh_get_cstring(name));
  
  {
    NEW(local_info, self);
    self->directory = directory;
    self->name = name;
    return self;
  }
}

void
lsh_popd(int old_cd, const char *directory)
{
  while (fchdir(old_cd) < 0)
    if (errno != EINTR)
      fatal("io.c: Failed to cd back from %z %e\n",
	    directory, errno);
      
  close(old_cd);
}

int
lsh_pushd_fd(int dir)
{
  int old_cd;

  /* cd to it, but first save old cwd */

  old_cd = open(".", O_RDONLY);
  if (old_cd < 0)
    {
      werror("io.c: open(`.') failed.\n");
      return -1;
    }

  io_set_close_on_exec(old_cd);

  /* Test if we are allowed to cd to our current working directory. */
  while (fchdir(old_cd) < 0)
    if (errno != EINTR)
      {
	werror("io.c: fchdir(`.') failed %e\n", errno);
	close(old_cd);
	return -1;
      }

  /* As far as I have been able to determine, all checks for
   * fchdir:ability is performed at the time the directory was opened.
   * Even if the directory is chmod:et to zero, or unlinked, we can
   * probably fchdir back to old_cd later. */

  while (fchdir(dir) < 0)
    if (errno != EINTR)
      {
	close(old_cd);
	return -1;
      }

  return old_cd;
}

/* Changes the cwd, making sure that it it has reasonable permissions,
 * and that we can change back later. */
int
lsh_pushd(const char *directory,
	  /* The fd to the directory is stored in *FD, unless fd is
	   * NULL */
	  int *result,
	  int create, int secret)
{
  int old_cd;
  int fd;
  struct stat sbuf;

  if (create)
    {  
      /* First create the directory, in case it doesn't yet exist. */
      if ( (mkdir(directory, 0700) < 0)
	   && (errno != EEXIST) )
	{
	  werror("io.c: Creating directory %z failed "
		 "%e\n", directory, errno);
	}
    }

  fd = open(directory, O_RDONLY);
  if (fd < 0)
    return -1;

  if (fstat(fd, &sbuf) < 0)
    {
      werror("io.c: Failed to stat `%z'.\n"
	     "  %e\n", directory, errno);
      return -1;
    }
  
  if (!S_ISDIR(sbuf.st_mode))
    {
      close(fd);
      return -1;
    }

  if (secret)
    {
      /* Check that it has reasonable permissions */
      if (sbuf.st_uid != getuid())
	{
	  werror("io.c: Socket directory %z not owned by user.\n", directory);

	  close(fd);
	  return -1;
	}
    
      if (sbuf.st_mode & (S_IRWXG | S_IRWXO))
	{
	  werror("io.c: Permission bits on %z are too loose.\n", directory);

	  close(fd);
	  return -1;
	}
    }
  
  /* cd to it, but first save old cwd */

  old_cd = open(".", O_RDONLY);
  if (old_cd < 0)
    {
      werror("io.c: open('.') failed.\n");

      close(fd);
      return -1;
    }

  io_set_close_on_exec(old_cd);

  /* Test if we are allowed to cd to our current working directory. */
  while (fchdir(old_cd) < 0)
    if (errno != EINTR)
      {
	werror("io.c: fchdir(\".\") failed %e\n", errno);
	close(fd);
	close(old_cd);
	return -1;
      }

  /* As far as I have been able to determine, all checks for
   * fchdir:ability is performed at the time the directory was opened.
   * Even if the directory is chmod:et to zero, or unlinked, we can
   * probably fchdir back to old_cd later. */

  while (fchdir(fd) < 0)
    {
      close(fd);
      close(old_cd);
      return -1;
    }

  if (result)
    *result = fd;
  else
    close(fd);
  
  return old_cd;
}


struct lsh_fd *
io_bind_local(struct local_info *info,
	      struct exception_handler *e)
{
  int old_cd;

  mode_t old_umask;
  struct sockaddr_un *local;
  socklen_t local_length;

  struct lsh_fd *fd;

  const char *cdir = lsh_get_cstring(info->directory);
  const char *cname = lsh_get_cstring(info->name);
  uint32_t length = lsh_string_length(info->name);
  
  assert(cdir);
  assert(cname);

  /* NAME should not be a plain filename, with no directory separators.
   * In particular, it should not be an absolute filename. */
  assert(!memchr(cname, '/', length));

  local_length = offsetof(struct sockaddr_un, sun_path) + length;
  local = alloca(local_length + 1);

  local->sun_family = AF_UNIX;
  memcpy(local->sun_path, cname, length);
  local->sun_path[length] = 0;
  
  /* cd to it, but first save old cwd */

  old_cd = lsh_pushd(cdir, NULL, 1, 1);
  if (old_cd < 0)
    return NULL;

  /* Ok, now the current directory should be a decent place for
   * creating a socket. */

  /* Try unlinking any existing file. */
  if ( (unlink(cname) < 0)
       && (errno != ENOENT))
    {
      werror("io.c: unlink '%S'/'%S' failed %e\n",
	     info->directory, info->name, errno);
      lsh_popd(old_cd, cdir);
      return NULL;
    }

  /* We have to change the umask, as that's the only way to control
   * the permissions that bind uses. */

  old_umask = umask(0077);

  /* Bind and listen */
  fd = io_bind_sockaddr((struct sockaddr *) local,
			local_length, e);
  
  /* Ok, now we restore umask and cwd */
  umask(old_umask);

  lsh_popd(old_cd, cdir);

  return fd;
}

/* Requires DIRECTORY and NAME to be NUL-terminated */
struct lsh_fd *
io_connect_local(struct local_info *info,
		 struct command_continuation *c,
		 struct exception_handler *e)
{
  int old_cd;

  struct sockaddr_un *addr;
  socklen_t addr_length;

  struct lsh_fd *fd;
  
  const char *cdir = lsh_get_cstring(info->directory);
  const char *cname = lsh_get_cstring(info->name);
  uint32_t length = lsh_string_length(info->name);

  assert(cname);
  assert(cdir);

  /* NAME should not be a plain filename, with no directory separators.
   * In particular, it should not be an absolute filename. */
  assert(!memchr(cname, '/', length));

  addr_length = offsetof(struct sockaddr_un, sun_path) + length;
  addr = alloca(addr_length);

  addr->sun_family = AF_UNIX;
  memcpy(addr->sun_path, cname, length);

  /* cd to it, but first save old cwd */

  old_cd = lsh_pushd(cdir, NULL, 0, 1);
  if (old_cd < 0)
    return NULL;
  
  fd = io_connect( (struct sockaddr *) addr, addr_length,
		   make_connect_callback(c), e);

  lsh_popd(old_cd, cdir);

  return fd;
}

/* Constructors */

struct lsh_fd *
io_read_write(struct lsh_fd *fd,
	      struct io_callback *read,
	      uint32_t block_size,
	      struct lsh_callback *close_callback)
{
  trace("io.c: Preparing fd %i for reading and writing\n",
	fd->fd);
  
  /* Reading */
  fd->read = read;
  if (read)
    lsh_oop_register_read_fd(fd);
  
  /* Writing */
  fd->write_buffer = make_write_buffer(fd, block_size);
  fd->write = &io_write_callback;

  /* Closing */
  fd->close_callback = close_callback;

  return fd;
}

struct lsh_fd *
io_read(struct lsh_fd *fd,
	struct io_callback *read,
	struct lsh_callback *close_callback)
{
  trace("io.c: Preparing fd %i for reading\n", fd->fd);
  
  /* Reading */
  fd->read = read;
  if (fd->read)
    lsh_oop_register_read_fd(fd);
  
  fd->close_callback = close_callback;

  return fd;
}

struct lsh_fd *
io_write(struct lsh_fd *fd,
	 uint32_t block_size,
	 struct lsh_callback *close_callback)
{
  trace("io.c: Preparing fd %i for writing\n", fd->fd);
  
  /* Writing */
  fd->write_buffer = make_write_buffer(fd, block_size);
  fd->write = &io_write_callback;

  fd->close_callback = close_callback;

  return fd;
}

/* Used e.g. for the key capture-file. Never closed. */
/* GABA:
   (class
     (name write_only_file)
     (super abstract_write)
     (vars
       (fd . int)
       (e object exception_handler)))
*/

static void
do_write_only_file(struct abstract_write *s, struct lsh_string *data)
{
  CAST(write_only_file, self, s);
  const struct exception *e;

  e = write_raw(self->fd, STRING_LD(data));

  if (e)
    EXCEPTION_RAISE(self->e, e);

  lsh_string_free(data);
}

struct abstract_write *
make_io_write_file(int fd, struct exception_handler *e)
{
    NEW(write_only_file, self);
    self->super.write = do_write_only_file;
    self->fd = fd;
    self->e = e;

    return &self->super;  
}

struct abstract_write *
io_write_file(const char *fname, int flags, int mode,
	      struct exception_handler *e)
{
  int fd = open(fname, flags, mode);
  if (fd < 0)
    return NULL;

  io_set_close_on_exec(fd);

  return make_io_write_file(fd, e);
}

void
close_fd(struct lsh_fd *fd)
{
  trace("io.c: Closing fd %i: %z, type %i.\n",
	fd->fd, fd->label, fd->type);

  if (fd->super.alive)
    {
      lsh_oop_cancel_read_fd(fd);
      lsh_oop_cancel_write_fd(fd);

      fd->super.alive = 0;

      /* fd->fd = -1 used to mean that we should unlink the file object,
	 but not close the underlying file. We don't do that anymore. */
      assert(fd->fd >= 0);
        
      /* Used by write fd:s to make sure that writing to its
       * buffer fails. */
      if (fd->write_buffer)
	fd->write_buffer->closed = 1;
      
      if (fd->close_callback)
	LSH_CALLBACK(fd->close_callback);

      if (fd->type != IO_STDERR)
	{
	  if (close(fd->fd) < 0)
	    {
	      werror("io.c: close failed %e\n", errno);
	      EXCEPTION_RAISE(fd->e,
			      make_io_exception(EXC_IO_CLOSE, fd,
						errno, NULL));
	    }
	  if (fd->type == IO_STDIO)
	    {
	      int null = open("/dev/null", O_RDWR);
	      if (null < 0)
		fatal("Failed to open /dev/null!\n");
	      if (null != fd->fd)
		fatal("Failed to map stdio fd %i to /dev/null.\n", fd->fd);
	    }
	}
      assert(nfiles);
      if (!--nfiles)
	lsh_oop_stop();
    }
  else
    werror("Closed already.\n");
}

void
close_fd_nicely(struct lsh_fd *fd)
{
  /* Don't attempt to read any further, and close the buffer as soon
   * as the write_buffer is empty. */

  trace("io.c: close_fd_nicely called on fd %i: %z\n",
	fd->fd, fd->label);

  /* Stop reading */
  fd->read = NULL;
  lsh_oop_cancel_read_fd(fd);

  if (fd->write_buffer)
    /* Close after currently buffered data is written out. */
    close_fd_write(fd);
  else
    /* There's no data buffered for write. */
    close_fd(fd);
}

/* Stop reading, but if the fd has a write callback, keep it open. */
void
close_fd_read(struct lsh_fd *fd)
{
  fd->read = NULL;

  lsh_oop_cancel_read_fd(fd);
  
  if (!fd->write)
    /* We won't be writing anything on this fd, so close it. */
    close_fd(fd);
}

/* Stop writing, but if the fd has a read callback, keep it open. */
void
close_fd_write(struct lsh_fd *fd)
{
  trace("io.c: close_fd_write called on fd %i: %z\n",
	fd->fd, fd->label);

  if (fd->write_buffer)
    {
      /* Mark the write_buffer as closed */
      fd->write_buffer->closed = 1;

      if (fd->type == IO_PTY)
	{
	  debug("Writing ^D to pty.\n");
	  /* Is there any better way to signal EOF on a pty? This is
	   * what emacs does. */
	  A_WRITE(&fd->write_buffer->super,
		  ssh_format("%lc", /* C-d */ 4));

	  /* No need to repeat this. */
	  fd->type = 0;
	}
      
      if (fd->write_buffer->empty)
	{
          if (!fd->read)
            close_fd(fd);
          else
            {
              /* Keep the fd open, but shutdown writing. */
              
              fd->write = NULL;
              lsh_oop_cancel_write_fd(fd);

              /* Try calling shutdown */
              if ( (shutdown (fd->fd, SHUT_WR) < 0)
                   && errno != ENOTSOCK)
                werror("close_fd_write, shutdown failed, %e\n", errno);
            }
        }
    }
  else
    if (!fd->read)
      close_fd(fd);
}

/* Responsible for handling the EXC_FINISH_READ exception. It should
 * be a parent to the connection related exception handlers, as for
 * instance the protocol error handler will raise the EXC_FINISH_READ
 * exception. */
/* GABA:
   (class
     (name exc_finish_read_handler)
     (super exception_handler)
     (vars
       (fd object lsh_fd)))
*/

static void
do_exc_finish_read_handler(struct exception_handler *s,
			   const struct exception *e)
{
  CAST(exc_finish_read_handler, self, s);
  switch(e->type)
    {
    case EXC_FINISH_READ:
      close_fd_nicely(self->fd);
      break;
    case EXC_FINISH_IO:
      close_fd(self->fd);
      break;
    case EXC_PAUSE_READ:
      lsh_oop_cancel_read_fd(self->fd);
      break;
    case EXC_PAUSE_START_READ:
      if (self->fd->read)
	lsh_oop_register_read_fd(self->fd);
      break;
    default:
      EXCEPTION_RAISE(self->super.parent, e);
    }
}

struct exception_handler *
make_exc_finish_read_handler(struct lsh_fd *fd,
			     struct exception_handler *parent,
			     const char *context)
{
  NEW(exc_finish_read_handler, self);

  self->super.parent = parent;
  self->super.raise = do_exc_finish_read_handler;
  self->super.context = context;
  
  self->fd = fd;

  return &self->super;
}

const struct exception finish_read_exception =
STATIC_EXCEPTION(EXC_FINISH_READ, "Stop reading");

const struct exception finish_io_exception =
STATIC_EXCEPTION(EXC_FINISH_IO, "Stop i/o");

struct exception *
make_io_exception(uint32_t type, struct lsh_fd *fd, int error, const char *msg)
{
  NEW(io_exception, self);
  assert(type & EXC_IO);
  
  self->super.type = type;

  if (msg)
    self->super.msg = msg;
  else
    self->super.msg = error ? strerror(error) : "Unknown i/o error";

  self->error = error;
  self->fd = fd;
  
  return &self->super;
}


/* Creates a one-way socket connection. Returns 1 on success, 0 on
 * failure. fds[0] is for reading, fds[1] for writing (like for the
 * pipe system call). */
int
lsh_make_pipe(int *fds)
{
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
      werror("socketpair failed %e\n", errno);
      return 0;
    }
  trace("Created socket pair. Using fd:s %i <-- %i\n", fds[0], fds[1]);

  if (SHUTDOWN_UNIX(fds[0], SHUT_WR_UNIX) < 0)
    {
      werror("shutdown(%i, SHUT_WR) failed %e\n", fds[0], errno);
      goto fail;
    }
  if (SHUTDOWN_UNIX(fds[1], SHUT_RD_UNIX) < 0)
    {
      werror("shutdown(%i, SHUT_RD_UNIX) failed %e\n", fds[0], errno);
    fail:
      {
	int saved_errno = errno;

	close(fds[0]);
	close(fds[1]);

	errno = saved_errno;
	return 0;
      }
    }
  
  return 1;
}

/* Forks a filtering process, and reads the output. Always closes
   the IN fd. */
int
lsh_popen(const char *program, const char **argv, int in,
	  pid_t *child)
{
  /* 0 for read, 1, for write */
  int out[2];
  pid_t pid;
  
  if (!lsh_make_pipe(out))
    {
      close(in);
      return -1;
    }
  pid = fork();

  if (pid < 0)
    {
      close(in);
      close(out[0]);
      close(out[1]);
      return -1;
    }
  else if (!pid)
    {
      /* Child */
      if (dup2(in, STDIN_FILENO) < 0)
	{
	  werror("lsh_popen: dup2 for stdin failed %e.\n", errno);
	  _exit(EXIT_FAILURE);
	}
      if (dup2(out[1], STDOUT_FILENO) < 0)
	{
	  werror("lsh_popen: dup2 for stdout failed %e.\n", errno);
	  _exit(EXIT_FAILURE);
	}

      close(in);
      close(out[0]);
      close(out[1]);

      /* The execv prototype uses const in the wrong way */
      execv(program, (char **) argv);

      werror("lsh_popen: execv `%z' failed %e.\n", program, errno);

      _exit(EXIT_FAILURE);
    }
  else
    {
      /* Parent process */
      close(in);
      close(out[1]);
      *child = pid;
      return out[0];
    }
}

struct lsh_string *
lsh_popen_read(const char *program, const char **argv, int in,
	       unsigned guess)
{
  pid_t pid;
  int status;
  int fd = lsh_popen(program, argv, in, &pid);
  struct lsh_string *s = io_read_file_raw(fd, guess);
  
  if (waitpid(pid, &status, 0) < 0)
    {
      werror("lsh_popen_read: waitpid failed: %e\n", errno);
      lsh_string_free(s);
      return NULL;
    }

  if (!s)
    return NULL;

  if (WIFEXITED(status))
    {
      if (!WEXITSTATUS(status))
	/* Success. */
	return s;

      werror("Program `%z' exited with status %i.\n",
	     program, WEXITSTATUS(status));
    }
  else
    werror("Program `%z' terminated by signal %i (%z).\n",
	   program, WTERMSIG(status), STRSIGNAL(WTERMSIG(status)));

  lsh_string_free(s);
  return NULL;
}

/* Copies data from one fd to another. Works no matter if the fd:s are
 * in blocking or non-blocking mode. Tries hard not to do premature
 * reads; we don't want to read data into the buffer, and then
 * discover that we can't write it out.
 *
 * The src fd may be the process stdin, and if we are too gready
 * reading it, we may consume data that belongs to the user's next
 * command. */
int
lsh_copy_file(int src, int dst)
{
#define BUF_SIZE 1024
  char buf[BUF_SIZE];
  struct pollfd src_poll;
  struct pollfd dst_poll;

  src_poll.fd = src;
  dst_poll.fd = dst;
  
  for (;;)
    {
      int res;
      uint32_t i, length;

      /* First wait until dst is writable; otherwise there's no point
       * in reading the input. */

      dst_poll.events = POLLOUT;

      do
	res = poll(&dst_poll, 1, -1);
      while ( (res < 0) && (errno == EINTR));

      debug("lsh_copy_file, initial poll on destination:\n"
	    "  res = %i, src = %i, dst = %i, events = %xi, revents = %xi.\n",
	    res, src, dst, dst_poll.events, dst_poll.revents);
      
      assert(res == 1);
      
      if (!(dst_poll.revents & POLLOUT))
	/* Most likely, dst is a pipe, and there are no readers. */
	return 1;

      /* Ok, can we read anything? */
      src_poll.events = MY_POLLIN;
      
      do
	res = poll(&src_poll, 1, -1);
      while ( (res < 0) && (errno == EINTR));
      
      if (res < 0)
	return 0;

      debug("lsh_copy_file, poll on src:\n"
	    "  res = %i, src = %i, dst = %i, events = %xi, revents = %xi.\n",
	    res, src, dst, src_poll.events, src_poll.revents);
      
      assert(res == 1);
      
      /* On linux, it seems that POLLHUP is set on read eof. */
      if (!(src_poll.revents & MY_POLLIN))
	/* EOF */
	return 1;

      assert(src_poll.revents & MY_POLLIN);

      /* Before actually reading anything, we need to check that
       * the dst fd is still alive. */

      dst_poll.events = POLLOUT;
      do
	res = poll(&dst_poll, 1, 0);
      while ( (res < 0) && (errno == EINTR));

      debug("lsh_copy_file, second poll on destination:\n"
	    "  res = %i, src = %i, dst = %i, events = %xi, revents = %xi.\n",
	    res, src, dst, dst_poll.events, dst_poll.revents);
      
      if (res && !(dst_poll.revents & POLLOUT))
	{
	  /* NOTE: Either somebody else filled up the buffer, or
	   * the fd is dead. How do we know which happened? We
	   * can't check POLLHUP, because it seems linux always
	   * sets it. As a kludge, and because this condition
	   * should be really rare, we check our ppid to see if
	   * the main process have died and left us to init. */

	  debug("lsh_copy_file: ppid = %i\n", getppid());
	  
	  if (getppid() == 1)
	    return 1;
	}

      do 
	res = read(src, buf, BUF_SIZE);
      while ( (res < 0) && (errno == EINTR));

      debug("lsh_copy_file: read on fd %i returned = %i\n", src, res);
      
      if (res < 0)
	return 0;
      else if (!res)
	/* EOF */
	return 1;

      length = res;

      for (i = 0; i<length; )
	{
	  dst_poll.events = POLLOUT;
	  do
	    res = poll(&dst_poll, 1, -1);
	  while ( (res < 0) && (errno == EINTR));

	  debug("lsh_copy_file, inner poll on destination:\n"
		"  res = %i, src = %i, dst = %i, events = %xi, revents = %xi.\n",
		res, src, dst, dst_poll.events, dst_poll.revents);
      
	  if (res < 0)
	    return 0;

	  assert(res == 1);
	  
	  if (!(dst_poll.revents & POLLOUT))
	    return 0;

	  do
	    res = write(dst, buf + i, length - i);
	  while ( (res < 0) && (errno == EINTR));

	  debug("lsh_copy_file: write on fd %i returned = %i\n", dst, res);
	  
	  if (res < 0)
	    return 0;

	  i += res;
	}
    }
#undef BUF_SIZE
}
