/**
 * @brief Generic repeated record collector
 *
 * Copyright (C) 2016 Red Hat
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _AUSHAPE_REP_COLL_H
#define _AUSHAPE_REP_COLL_H

#include <aushape/coll_type.h>

struct aushape_rep_coll_args {
    /** Name of the output container */
    const char *name;
};

/**
 * Generic repeated record collector type.
 *
 * Expects a pointer to struct aushape_rep_coll_args as the creation
 * argument. Cannot be passed NULL, has no default.
 *
 * No collector-specific return codes.
 */
extern const struct aushape_coll_type aushape_rep_coll_type;

#endif /* _AUSHAPE_REP_COLL_H */
