/* lock_file.h
 *
 * Traditional O_EXCL-style file locking.
 *
 * $id:$
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lsh.h"

struct lsh_file_lock_info;

#define GABA_DECLARE
#include "lock_file.h.x"
#undef GABA_DECLARE

/* GABA:
   (class
     (name lsh_file_lock_info)
     (vars
       (lockname string)
       (lock method "struct resource *" "unsigned retries")
       (lock_p method int)))
*/

#define LSH_FILE_LOCK(i, r) ((i)->lock((i), (r)))
#define LSH_FILE_LOCK_P(i) ((i)->lock_p((i)))

/* Takes the name of the lock-file as argument.
 *
 * FIXME: Perhaps it would be better to take just the filename,
 * without ".lock"-suffix? */
struct lsh_file_lock_info *
make_lsh_file_lock_info(struct lsh_string *name);
