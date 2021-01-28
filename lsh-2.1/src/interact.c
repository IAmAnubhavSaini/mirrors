/* interact.c
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

#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include <fcntl.h>

#include "interact.h"

#include "io.h"
#include "werror.h"
#include "xalloc.h"

#define GABA_DEFINE
#include "interact.h.x"
#undef GABA_DEFINE

struct interact_dialog *
make_interact_dialog(unsigned nprompt)
{
  NEW(interact_dialog, self);
  unsigned i;

  self->instruction = NULL;
  self->nprompt = nprompt;
  self->prompt = lsh_space_alloc(nprompt * sizeof(struct lsh_string *));
  self->response = lsh_space_alloc(nprompt * sizeof(struct lsh_string *));
  self->echo = lsh_space_alloc(nprompt * sizeof(int));
  
  for (i = 0; i < nprompt; i++)
    self->prompt[i] = self->response[i] = NULL;

  return self;
}
