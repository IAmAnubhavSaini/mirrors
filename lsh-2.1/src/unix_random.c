/* unix_random.c
 *
 * Randomness polling on unix, using yarrow and ideas from Peter
 * Gutmann's cryptlib. */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2000, 2001 Niels Möller
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

#include <fcntl.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/time.h> /* Must be included before sys/resource.h */
#include <sys/resource.h>
#include <sys/stat.h>

#include <nettle/yarrow.h>

#include "randomness.h"

#include "crypto.h"
#include "environ.h"
#include "format.h"
#include "lock_file.h"
#include "lsh_string.h"
#include "reaper.h"
#include "xalloc.h"
#include "werror.h"

#include "unix_random.c.x"

/* GABA:
   (class
     (name unix_random)
     (super randomness)
     (vars
       (seed_file_fd . int)
       (lock object lsh_file_lock_info)
       (yarrow . "struct yarrow256_ctx")
       (sources array "struct yarrow_source" RANDOM_NSOURCES)

       ; For the SOURCE_TRIVIA, count the number of invocations per second
       (previous_time . long)
       (time_count . unsigned)

       ; For SOURCE_DEVICE
       (device_fd . int)
       (device_last_read . time_t)))
*/

static struct unix_random *the_random = NULL;

static int
write_seed_file(struct yarrow256_ctx *ctx,
	       int fd)
{
  const struct exception *e;
  uint8_t seed[YARROW256_SEED_FILE_SIZE];
  
  if (lseek(fd, 0, SEEK_SET) < 0)
    {
      werror("Seeking to beginning of seed file failed!? %e\n", errno);
      return 0;
    }

  yarrow256_random(ctx, sizeof(seed), seed);
  e = write_raw(fd, sizeof(seed), seed);

  if (e)
    {
      werror("Overwriting seed file failed: %z\n",
	     e->msg);
      return 0;
    }

  return 1;
}

static struct lsh_string *
read_seed_file(int fd)
{
  struct lsh_string *seed;
  
  if (lseek(fd, 0, SEEK_SET) < 0)
    {
      werror("Seeking to beginning of seed file failed!? %e\n", errno);
      return NULL;
    }

  seed = io_read_file_raw(fd, YARROW256_SEED_FILE_SIZE + 1);
  if (!seed)
    werror("Couldn't read seed file %e\n", errno);
  return seed;
}

static int
read_initial_seed_file(struct yarrow256_ctx *ctx,
		       int fd)
{
  struct lsh_string *seed;
  struct stat sbuf;

  if (fstat(fd, &sbuf) < 0)
    {
      werror("Couldn't stat seed file %e\n", errno);
      return 0;
    }

  /* Check permissions */
  if (sbuf.st_uid != getuid())
    {
      werror("Seed file not owned by you!\n");
      return 0;
    }

  if (sbuf.st_mode & (S_IRWXG | S_IRWXO))
    {
      werror("Too permissive permissions on seed-file.\n");
      return 0;
    }
  
  seed = read_seed_file(fd);
  if (!seed)
    return 0;
  
  if (lsh_string_length(seed) < YARROW256_SEED_FILE_SIZE)
    {
      werror("Seed file too short\n");
      lsh_string_free(seed);
      return 0;
    }
  
  yarrow256_seed(ctx, STRING_LD(seed));
  lsh_string_free(seed);

  assert(yarrow256_is_seeded(ctx));

  if (lseek(fd, 0, SEEK_SET) < 0)
    {
      werror("Seeking to beginning of seed file failed!? %e\n", errno);
      return 0;
    }

  return 1;
}

static void
update_seed_file(struct unix_random *self)
{
  struct resource *lock;
  verbose("Overwriting seed file.\n");

  lock = LSH_FILE_LOCK(self->lock, 0);
  if (!lock)
    {
      werror("Failed to lock seed file, so not overwriting it now.\n");
    }
  else
    {
      struct lsh_string *s = read_seed_file(self->seed_file_fd);

      /* Mix in the old seed file, it might have picked up
       * some randomness. */
      if (s)
	{
	  self->yarrow.sources[RANDOM_SOURCE_NEW_SEED].next = YARROW_FAST;
	  yarrow256_update(&self->yarrow, RANDOM_SOURCE_NEW_SEED,
			   0, STRING_LD(s));
	  lsh_string_free(s);
	  yarrow256_fast_reseed(&self->yarrow);
	}

      write_seed_file(&self->yarrow, self->seed_file_fd);
      KILL_RESOURCE(lock);
    }
}

static int
do_trivia_source(struct unix_random *self, int init)
{
  struct {
    struct timeval now;
#if HAVE_GETRUSAGE
    struct rusage rusage;
#endif
    unsigned count;
    pid_t pid;
  } event;
  
  unsigned entropy = 0;

  if (gettimeofday(&event.now, NULL) < 0)
    fatal("gettimeofday failed %e\n", errno);
#if HAVE_GETRUSAGE
  if (getrusage(RUSAGE_SELF, &event.rusage) < 0)
    fatal("getrusage failed %e\n", errno);
#endif

  event.count = 0;
  if (init)
    {
      self->time_count = 0;
    }
  else
    {
      event.count = self->time_count++;
      
      if (event.now.tv_sec != self->previous_time)
	{
	  /* Count one bit of entropy if we either have more than two
	   * invocations in one second, or more than two seconds
	   * between invocations. */
	  if ( (self->time_count > 2)
	       || ( (event.now.tv_sec - self->previous_time) > 2) )
	    entropy++;

	  self->time_count = 0;
	}
    }
  self->previous_time = event.now.tv_sec;
  event.pid = getpid();

  return yarrow256_update(&self->yarrow, RANDOM_SOURCE_TRIVIA, entropy,
			  sizeof(event), (const uint8_t *) &event);
}

#define DEVICE_READ_SIZE 10
static int
do_device_source(struct unix_random *self, int init)
{
  time_t now = time(NULL);
  
  if (init)
    {
      self->device_fd = open("/dev/urandom", O_RDONLY);
      if (self->device_fd < 0)
	return 0;

      io_set_close_on_exec(self->device_fd);
      self->device_last_read = now;
    }

  if ( (self->device_fd > 0)
       && (init || ( (now - self->device_last_read) > 60)))
    {
      /* More than a minute since we last read the device */
      uint8_t buf[DEVICE_READ_SIZE];
      const struct exception *e
	= read_raw(self->device_fd, sizeof(buf), buf);

      if (e)
	{
	  werror("Failed to read /dev/urandom %e\n", errno);
	  return 0;
	}

      self->device_last_read = now;
      
      return yarrow256_update(&self->yarrow, RANDOM_SOURCE_DEVICE,
			      10, /* Estimate 10 bits of entropy */
			      sizeof(buf), buf);
    }
  return 0;
}
#undef DEVICE_READ_SIZE


static void
do_unix_random(struct randomness *s,
	       uint32_t length,
	       uint8_t *dst)
{
  CAST(unix_random, self, s);

  /* First do a "fast" poll. */
  int trivia_reseed = do_trivia_source(self, 0);
  int device_reseed = do_device_source(self, 0);

  if (trivia_reseed || device_reseed)
    update_seed_file(self);

  /* Ok, generate some output */
  yarrow256_random(&self->yarrow, length, dst);
}

static void
do_unix_random_add(struct randomness *s,
		   enum random_source_type type,
		   uint32_t length,
		   const uint8_t *data)
{
  CAST(unix_random, self, s);

  unsigned entropy;
  
  switch(type)
    {
    case RANDOM_SOURCE_SECRET:
      /* Count one bit of entropy per character in a password or
       * key */
      entropy = length;
      break;
    case RANDOM_SOURCE_REMOTE:
      /* Count one bit of entropy if we have two bytes of padding. */
      entropy = (length >= 2);
      break;

    default:
      fatal("Internal error\n");
    }

  if (yarrow256_update(&self->yarrow, type,
		       entropy,
		       length, data))
    update_seed_file(self);
}

struct randomness *
random_init(struct lsh_string *seed_file_name)
{
  assert(!the_random);

  trace("random_init\n");
  {
    NEW(unix_random, self);
    struct resource *lock;

    self->super.quality = RANDOM_GOOD;
    self->super.random = do_unix_random;
    self->super.add = do_unix_random_add;

    yarrow256_init(&self->yarrow, RANDOM_NSOURCES, self->sources);
    
    if (access(lsh_get_cstring(seed_file_name), F_OK) < 0)
      {
	werror("No seed file. Please create one by running\n");
	werror("lsh-make-seed -o \"%S\".\n", seed_file_name);

	KILL(self);
	return NULL;
      }

    verbose("Reading seed-file `%S'\n", seed_file_name);
    
    self->lock
      = make_lsh_file_lock_info(ssh_format("%lS.lock", seed_file_name));

    trace("random_init, locking seed file...\n");
    
    lock = LSH_FILE_LOCK(self->lock, 5);

    if (!lock)
      {
	werror("Could not lock seed-file `%S'\n", seed_file_name);
	KILL(self);
	return NULL;
      }

    trace("random_init, seed file locked successfully.\n");
    
    self->seed_file_fd = open(lsh_get_cstring(seed_file_name), O_RDWR);
    if (self->seed_file_fd < 0)
      {
	werror("Could not open seed file \"%S\".\n", seed_file_name);

	KILL_RESOURCE(lock);
	KILL(self);
	return NULL;
      }

    io_set_close_on_exec(self->seed_file_fd);
    trace("random_init, reading seed file...\n");
    
    if (!read_initial_seed_file(&self->yarrow, self->seed_file_fd))
      {
	KILL_RESOURCE(lock);
	KILL(self);
	return NULL;
      }

    trace("random_init, seed file read successfully.\n");
    
    assert(yarrow256_is_seeded(&self->yarrow));
    
    /* Ok, now yarrow is initialized. */
    do_device_source(self, 1);
    do_trivia_source(self, 1);

    /* Mix that data in before generating any output. */
    yarrow256_force_reseed(&self->yarrow);

    /* Overwrite seed file. */
    if (!write_seed_file(&self->yarrow, self->seed_file_fd))
      {
	KILL_RESOURCE(lock);
	KILL(self);
	return NULL;
      }

    trace("random_init: All done, releasing lock.\n");

    KILL_RESOURCE(lock);
    
    the_random = self;
    return &self->super;
  }
}

struct randomness *
make_user_random(const char *home)
{
  struct randomness *r;
  struct lsh_string *file_name;
  const char *env_name;

  env_name = getenv(ENV_SEED_FILE);
  if (env_name)
    file_name = make_string(env_name);
  else
    {
      if (!home)
	{
	  werror("Please set HOME in your environment.\n");
	  return NULL;
	}

      file_name = ssh_format("%lz/.lsh/yarrow-seed-file", home);
    }
  r = random_init(file_name);
  
  lsh_string_free(file_name);

  return r;
}

struct randomness *
make_system_random(void)
{
  struct randomness *r;
  struct lsh_string *file_name;
  
  const char *env_name;

  env_name = getenv(ENV_SEED_FILE);

  /* FIXME: What's a proper place for this? */
  file_name = make_string(env_name ? env_name
			  : "/var/spool/lsh/yarrow-seed-file");

  r = random_init(file_name);
  
  lsh_string_free(file_name);

  return r;
}
