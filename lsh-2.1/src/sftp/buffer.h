/* buffer.h
 *
 * Buffering for sftp.
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2001 Niels Möller
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#ifndef SFTP_BUFFER_H_INCLUDED
#define SFTP_BUFFER_H_INCLUDED

/* For off_t */
#include <sys/types.h>

/* For uint32_t and friends */
#include <nettle/nettle-types.h>



/* Abstract input and output functions */
#include <time.h>

struct sftp_input;
struct sftp_output;

/* Input */

/* Returns 1 of all was well, 0 on error, and -1 on EOF */
int
sftp_read_packet(struct sftp_input *i);

int
sftp_check_input(const struct sftp_input *i, uint32_t length);

int
sftp_get_data(struct sftp_input *i, uint32_t length, uint8_t *data);

int
sftp_get_uint8(struct sftp_input *i, uint8_t *value);

int
sftp_get_uint32(struct sftp_input *i, uint32_t *value);

int
sftp_get_uint64(struct sftp_input *i, off_t *value);

/* Allocates storage. Caller must deallocate using
 * sftp_free_string. */
uint8_t *
sftp_get_string(struct sftp_input *i, uint32_t *length);

void
sftp_free_string(uint8_t *s);

/* Like sftp_get_string, but the data is deallocated automatically by
 * sftp_read_packet and sftp_input_clear_strings. */
uint8_t *
sftp_get_string_auto(struct sftp_input *i, uint32_t *length);

void
sftp_input_clear_strings(struct sftp_input *i);

void
sftp_input_remember_string(struct sftp_input *i, uint8_t *s);

int
sftp_get_eod(struct sftp_input *i);

/* Output */

void
sftp_set_msg(struct sftp_output *o, uint8_t msg);

void
sftp_set_id(struct sftp_output *o, uint32_t id);

int
sftp_write_packet(struct sftp_output *o);

void
sftp_put_data(struct sftp_output *o, uint32_t length, const uint8_t *data);

void
sftp_put_uint8(struct sftp_output *o, uint8_t value);

void
sftp_put_uint32(struct sftp_output *o, uint32_t value);

void
sftp_put_uint64(struct sftp_output *o, off_t value);

void
sftp_put_string(struct sftp_output *o, uint32_t length, const uint8_t *data);

/* Low-level functions, used by sftp_put_printf and sftp_put_strftime */
uint8_t *
sftp_put_start(struct sftp_output *o, uint32_t length);

void
sftp_put_end(struct sftp_output *o, uint32_t length);

/* Returns index. */
uint32_t
sftp_put_reserve_length(struct sftp_output *o);

void
sftp_put_final_length(struct sftp_output *o,
		      uint32_t index);

void
sftp_put_length(struct sftp_output *o,
		uint32_t index,
		uint32_t length);

void
sftp_put_reset(struct sftp_output *o,
	       uint32_t index);

uint32_t
sftp_put_printf(struct sftp_output *o, const char *format, ...)
     PRINTF_STYLE(2,3);
     
uint32_t
sftp_put_strftime(struct sftp_output *o, uint32_t size,
		  const char *format,
		  const struct tm *tm);

int
sftp_packet_size(struct sftp_output* out);

/* Constructed types. */

struct sftp_attrib
{
  uint32_t flags;
  off_t size;
  uint32_t uid;
  uint32_t gid;
  uint32_t permissions;

  /* NOTE: The representations of times is about to change. */
  uint32_t atime;
  uint32_t mtime;
};

void
sftp_clear_attrib(struct sftp_attrib *a);

int
sftp_get_attrib(struct sftp_input *i, struct sftp_attrib *a);

void
sftp_put_attrib(struct sftp_output *o, const struct sftp_attrib *a);

int
sftp_skip_extension(struct sftp_input *i);



/* Macros */
/* Reads a 32-bit integer, in network byte order */
#define READ_UINT32(p)				\
(  (((uint32_t) (p)[0]) << 24)			\
 | (((uint32_t) (p)[1]) << 16)			\
 | (((uint32_t) (p)[2]) << 8)			\
 |  ((uint32_t) (p)[3]))

#define WRITE_UINT32(p, i)			\
do {						\
  (p)[0] = ((i) >> 24) & 0xff;			\
  (p)[1] = ((i) >> 16) & 0xff;			\
  (p)[2] = ((i) >> 8) & 0xff;			\
  (p)[3] = (i) & 0xff;				\
} while(0)

#endif /* SFTP_BUFFER_H_INCLUDED */
