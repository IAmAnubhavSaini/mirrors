/* unix_interact.c
 *
 * Interact with the user.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1999 Niels Möller
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

#include <signal.h>

#include <fcntl.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include "interact.h"

#include "format.h"
#include "io.h"
#include "lsh_string.h"
#include "resource.h"
#include "suspend.h"
#include "tty.h"
#include "werror.h"
#include "xalloc.h"

#include "unix_interact.c.x"

/* Depends on the tty being line buffered. FIXME: Doesn't distinguish
   between errors, empty input, and EOF. */
static uint32_t
read_line(int fd, uint32_t size, uint8_t *buffer)
{
  uint32_t i = 0;

  while (i < size)
    {
      int res = read(fd, buffer + i, size - i);
      if (!res)
	/* User pressed EOF (^D) */
	return i;
      else if (res < 0)
	switch(errno)
	  {
	  case EAGAIN:
	  case EINTR:
	    break;
	  default:
	    /* I/O error */
	    werror("unix_interact.c: read_line, %e\n", errno);
	    return 0;
	  }
      else
	{
	  uint32_t j;
	  for (j = 0; j < (unsigned) res; j++, i++)
	    if (buffer[i] == '\n')
	      return i;
	}
    }
  /* We have filled our buffer already; continue reading until end of line */
#define BUFSIZE 512
  for (;;)
    {
      uint8_t b[BUFSIZE];
      int res = read(fd, b, BUFSIZE);
      if (!res)
	/* EOF */
	return size;
      else if (res < 0)
	switch(errno)
	  {
	  case EAGAIN:
	  case EINTR:
	    break;
	  default:
	    /* I/O error */
	    werror("tty_read_line %e\n", errno);
	    return 0;
	  }
      else
	{
	  uint32_t j;
	  for (j = 0; j < (unsigned) res; j++)
	    if (b[j] == '\n')
	      return size;
	}
    }
#undef BUFSIZE
}


/* NOTE: If there are more than one instance of this class, window
 * changes can be missed, as only one of them will have its signal
 * handler installed properly. */

/* GABA:
   (class
     (name window_subscriber)
     (super resource)
     (vars
       (interact object unix_interact)
       (next object window_subscriber)
       (callback object window_change_callback)))
*/

static void
do_kill_window_subscriber(struct resource *s)
{
  CAST(window_subscriber, self, s);

  if (self->super.alive)
    {
      self->super.alive = 0;
      assert(self->interact->nsubscribers);
      assert(self->interact->winch_handler);
      
      if (!--self->interact->nsubscribers)
        {
          KILL_RESOURCE(self->interact->winch_handler);
          self->interact->winch_handler = NULL;
        }
    }
}

/* FIXME: Remove subscribers list, and register multiple liboop signal
 * handlers instead? */
/* GABA:
   (class
     (name unix_interact)
     (super interact)
     (vars
       (tty_fd . int)
       (askpass . "const char *")
       ; Signal handler
       (winch_handler object resource)
       (nsubscribers . unsigned)
       (subscribers object window_subscriber)))
*/

#define IS_TTY(self) ((self)->tty_fd >= 0)

static int
unix_is_tty(struct interact *s)
{
  CAST(unix_interact, self, s);

  return IS_TTY(self);
}

/* FIXME: Rewrite to operate on tty_fd. */
static struct lsh_string *
read_password(struct unix_interact *self,
	      const struct lsh_string *prompt)
{
  if (self->askpass)
    {
      const char *argv[3];
      
      int null = open("/dev/null", O_RDONLY);
      
      if (null < 0)
	{
	  werror("Failed to open /dev/null!\n");
	  
	  return NULL;
	}
      
      argv[0] = self->askpass;
      argv[1] = lsh_get_cstring(prompt);
      if (!argv[1])
	{
	  close(null);

	  return NULL;
	}
      argv[2] = NULL;

      trace("unix_interact.c: spawning askpass program `%z'\n",
	    self->askpass);

      return lsh_popen_read(self->askpass, argv, null, 100);
    }
  else
    {
      /* NOTE: Ignores max_length; instead getpass's limit applies. */
  
      char *password;
      const char *cprompt;

      if (!IS_TTY(self))
	return NULL;

      cprompt = lsh_get_cstring(prompt);
      if (!cprompt)
	return NULL;

      /* NOTE: This function uses a static buffer. */
      password = getpass(cprompt);
  
      if (!password)
	return NULL;
  
      return make_string(password);
    }
}

static struct lsh_string *
unix_read_password(struct interact *s,
		   uint32_t max_length UNUSED,
		   const struct lsh_string *prompt)
{
  CAST(unix_interact, self, s);
  struct lsh_string *password;
  
  trace("unix_interact.c:unix_read_password\n");

  password = read_password(self, prompt);
  lsh_string_free(prompt);
  
  return password;
}

static void
unix_set_askpass(struct interact *s,
		 const char *askpass)
{
  CAST(unix_interact, self, s);
  trace("unix_interact.c:unix_set_askpass\n");
  assert(askpass);
  
  self->askpass = askpass;
}

static int
unix_yes_or_no(struct interact *s,
	       const struct lsh_string *prompt,
	       int def, int free)
{
#define TTY_BUFSIZE 10

  CAST(unix_interact, self, s);
  if (!IS_TTY(self) || quiet_flag)
    {
      if (free)
	lsh_string_free(prompt);
      return def;
    }
  else    
    {
      uint8_t buffer[TTY_BUFSIZE];
      const struct exception *e;
  
      e = write_raw(self->tty_fd, STRING_LD(prompt));

      if (free)
	lsh_string_free(prompt);

      if (e)
	return def;

      if (!read_line(self->tty_fd, TTY_BUFSIZE, buffer))
	return def;

      switch (buffer[0])
	{
	case 'y':
	case 'Y':
	  return 1;
	default:
	  return 0;
	}
#undef TTY_BUFSIZE
    }
}

/* The prompts are typically not trusted, but it's the callers
   responsibility to sanity check them. */
static int
unix_dialog(struct interact *s,
	    const struct interact_dialog *dialog)
{
#define DIALOG_BUFSIZE 150
  CAST(unix_interact, self, s);
  const struct exception *e;
  unsigned i;
  
  e = write_raw(self->tty_fd, STRING_LD(dialog->instruction));

  if (e)
    return 0;

  for (i = 0; i < dialog->nprompt; i++)
    {
      struct lsh_string *prompt = dialog->prompt[i];
      if (dialog->echo[i])
	{
	  uint8_t buffer[DIALOG_BUFSIZE];
	  uint32_t length;
	  
	  e = write_raw(self->tty_fd, STRING_LD(prompt));
	  if (e)
	    return 0;
	  length = read_line(self->tty_fd, DIALOG_BUFSIZE, buffer);
	  if (!length)
	    return 0;
	  
	  dialog->response[i] = ssh_format("%ls", length, buffer);
	}
      else
	{
	  if (!(dialog->response[i] = read_password(self, prompt)))
	    return 0;
	}
    }
  return 1;
}


/* GABA:
   (class
     (name unix_termios)
     (super terminal_attributes)
     (vars
       (ios . "struct termios")))
*/

static struct terminal_attributes *
do_make_raw(struct terminal_attributes *s)
{
  CAST(unix_termios, self, s);
  CLONED(unix_termios, res, self);

  CFMAKERAW(&res->ios);

  /* Modify VMIN and VTIME, to save some bandwidth and make traffic
   * analysis of interactive sessions a little harder. */
  res->ios.c_cc[VMIN] = 4;
  /* Inter-character timer, in units of 0.1s */
  res->ios.c_cc[VTIME] = 1;
  
  return &res->super;
}

static struct lsh_string *
do_encode(struct terminal_attributes *s)
{
  CAST(unix_termios, self, s);
  return tty_encode_term_mode(&self->ios);
}

static struct terminal_attributes *
unix_get_attributes(struct interact *s)
{
  CAST(unix_interact, self, s);

  if (!IS_TTY(self))
    return NULL;
  else
    {
      NEW(unix_termios, res);
      res->super.make_raw = do_make_raw;
      res->super.encode = do_encode;
      
      if (!tty_getattr(self->tty_fd, &res->ios) < 0)
	{
	  KILL(res);
	  return NULL;
	}
      return &res->super;
    }
}

static int
unix_set_attributes(struct interact *s,
		    struct terminal_attributes *a)
{
  CAST(unix_interact, self, s);
  CAST(unix_termios, attr, a);

  return IS_TTY(self)
    && tty_setattr(self->tty_fd, &attr->ios);
}

static int
unix_window_size(struct interact *s,
		 struct terminal_dimensions *d)
{
  CAST(unix_interact, self, s);

  return IS_TTY(self)
    && tty_getwinsize(self->tty_fd, d);
}


/* GABA:
   (class
     (name winch_handler)
     (super lsh_callback)
     (vars
       (interact object unix_interact)))
*/

static void
do_winch_handler(struct lsh_callback *s)
{
  CAST(winch_handler, self, s);
  struct unix_interact *i = self->interact;

  assert(!!i->nsubscribers == !!i->winch_handler);
  
  if (i->subscribers)
    {
      struct window_subscriber *s;
      struct window_subscriber **s_p;
      unsigned alive;

      for (alive = 0, s_p = &i->subscribers; (s = *s_p) ;)
	{
	  if (!s->super.alive)
	    *s_p = s->next;
	  else
	    {
	      WINDOW_CHANGE_CALLBACK(s->callback, &i->super);
	      s_p = &s->next;
              alive++;
	    }
	}

      assert(alive == i->nsubscribers);
    }
}

static struct lsh_callback *
make_winch_handler(struct unix_interact *i)
{
  NEW(winch_handler, self);
  self->super.f = do_winch_handler;
  self->interact = i;

  return &self->super;
}

static struct resource *
unix_window_change_subscribe(struct interact *s,
			     struct window_change_callback *callback)
{
  CAST(unix_interact, self, s);

  NEW(window_subscriber, subscriber);

  init_resource(&subscriber->super, do_kill_window_subscriber);

  subscriber->interact = self;
  subscriber->next = self->subscribers;
  subscriber->callback = callback;

  self->subscribers = subscriber;
  self->nsubscribers++;
  
  if (!self->winch_handler)
    {
      /* This is the first subscriber */
      self->winch_handler
        = io_signal_handler(SIGWINCH,
                            make_winch_handler(self));
    }
  
  return &subscriber->super;
}

struct interact *
make_unix_interact(void)
{
  NEW(unix_interact, self);
  
  self->super.is_tty = unix_is_tty;
  self->super.read_password = unix_read_password;
  self->super.set_askpass = unix_set_askpass;
  self->super.yes_or_no = unix_yes_or_no;
  self->super.dialog = unix_dialog;
  self->super.get_attributes = unix_get_attributes;
  self->super.set_attributes = unix_set_attributes;
  self->super.window_size = unix_window_size;
  self->super.window_change_subscribe = unix_window_change_subscribe;

  self->tty_fd = -1;
  self->askpass = NULL;
  
#if HAVE_STDTTY_FILENO
  if (isatty(STDTTY_FILENO))
    self->tty_fd = STDTTY_FILENO;
#else /* ! HAVE_STDTTY_FILENO */
  self->tty_fd = open("/dev/tty", O_RDWR);
#endif

  if (self->tty_fd >= 0)
    {
      io_set_close_on_exec(self->tty_fd);
      /* Restore and reset tty if process is suspended. */      
      suspend_handle_tty(self->tty_fd);
    }
  return &self->super;
}
