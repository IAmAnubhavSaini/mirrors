/* parse_config.h
 *
 * Parsing of configuration files. */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2002 Niels Möller
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

#ifndef LSH_PARSE_CONFIG_H_INCLUDED
#define LSH_PARSE_CONFIG_H_INCLUDED

#include "lsh.h"

enum config_type
  { CONFIG_ADDRESS, CONFIG_USER };

struct config_setting;
struct config_group;

struct config_group *
config_parse_string(uint32_t length, const uint8_t *data);

struct config_match
{
  const struct config_setting *group;
  const struct config_setting *host;
};

int
config_lookup_host(const struct config_group *config,
		   const char *host,
		   struct config_match *match);

const struct lsh_string *
config_get_setting(enum config_type type,
		   const struct config_match *match);

#endif /* LSH_PARSE_CONFIG_H_INCLUDED */
