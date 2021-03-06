/* interact.h
 *
 * Interact with the user.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1999 Niels M�ller
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

#ifndef LSH_INTERACT_H_INCLUDED
#define LSH_INTERACT_H_INCLUDED

#include "lsh.h"

/* Forward declaration */
struct interact;
struct terminal_dimensions;

#define GABA_DECLARE
#include "interact.h.x"
#undef GABA_DECLARE

/* Abstract class defining methods needed to communicate with the
 * user's terminal. */

/* GABA:
   (class
     (name terminal_attributes)
     (vars
       (make_raw method (object terminal_attributes))
       (encode method (string))))
*/

#define TERM_MAKE_RAW(t) ((t)->make_raw((t)))
#define TERM_ENCODE(t) ((t)->encode((t)))

/* GABA:
   (class
     (name window_change_callback)
     (vars
       (f method void "struct interact *i")))
*/

#define WINDOW_CHANGE_CALLBACK(c, i) ((c)->f((c), (i)))

/* GABA:
   (class
     (name interact_dialog)
     (vars
       (instruction string)
       (nprompt . unsigned)
       (prompt space (string) nprompt)
       (response space (string) nprompt)
       (echo space int nprompt)))
*/

struct interact_dialog *
make_interact_dialog(unsigned nprompt);

/* GABA:
   (class
     (name interact)
     (vars
       (is_tty method int)
       ; Consumes the prompt
       (read_password method (string)
                  "uint32_t max_length"
		  "const struct lsh_string *prompt")
       (set_askpass method void "const char *askpass")
       (yes_or_no method int
                  "const struct lsh_string *prompt"
		  "int def" "int free")
       (dialog method int
		  "const struct interact_dialog *dialog")

       (get_attributes method (object terminal_attributes) )
       (set_attributes method int "struct terminal_attributes *attr")
       
       (window_size method int "struct terminal_dimensions *")
       (window_change_subscribe method (object resource)
		  "struct window_change_callback *callback")))
*/

#define INTERACT_IS_TTY(i) \
  ((i)->is_tty((i)))
#define INTERACT_READ_PASSWORD(i, l, p) \
  ((i)->read_password((i), (l), (p)))
#define INTERACT_SET_ASKPASS(i, a) \
  ((i)->set_askpass((i), (a)))
#define INTERACT_YES_OR_NO(i, p, d, f) \
  ((i)->yes_or_no((i), (p), (d), (f)))
#define INTERACT_DIALOG(i, d) \
  ((i)->dialog((i), (d)))
#define INTERACT_GET_ATTRIBUTES(i) \
  ((i)->get_attributes((i)))
#define INTERACT_SET_ATTRIBUTES(i, t) \
  ((i)->set_attributes((i), (t)))
#define INTERACT_WINDOW_SIZE(i, d) \
  ((i)->window_size((i), (d)))
#define INTERACT_WINDOW_SUBSCRIBE(i, c) \
  ((i)->window_change_subscribe((i), (c)))

struct interact *
make_unix_interact(void);

#endif /* LSH_INTERACT_H_INCLUDED */
