/**
 * @brief Unique (non-aggregated) record collector
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

#ifndef _AUSHAPE_UNIQ_COLL_H
#define _AUSHAPE_UNIQ_COLL_H

#include <aushape/coll_type.h>

/**
 * Unique (non-aggregated) record collector type.
 *
 * Doesn't require creation arguments.
 *
 * Collector-specific return codes:
 *
 * aushape_coll_add:
 *      AUSHAPE_RC_REPEATED_RECORD  - an unexpected repeated record
 *                                    type encountered
 */
extern const struct aushape_coll_type aushape_uniq_coll_type;

#endif /* _AUSHAPE_UNIQ_COLL_H */
