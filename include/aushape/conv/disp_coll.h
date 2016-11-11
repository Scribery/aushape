/**
 * @brief Dispatching record collector
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

#ifndef _AUSHAPE_CONV_DISP_COLL_H
#define _AUSHAPE_CONV_DISP_COLL_H

#include <aushape/conv/coll_type.h>

/** Record type name -> collector type link */
struct aushape_conv_disp_coll_type_link {
    /** Record type name */
    const char                             *name;
    /** Collector type */
    const struct aushape_conv_coll_type    *type;
    /** Collector creation arguments */
    const void                             *args;
};

/**
 * Dispatching record collector type.
 *
 * Expects an array of struct aushape_conv_disp_coll_type_link as the creation
 * argument. The array is terminated by an entry with NULL name, but valid
 * type and args. The array is used to match added record names to specific
 * collectors, the collector in the terminating entry is supplied with records
 * that didn't match any other collector. If the supplied argument is NULL,
 * then assumes it was supplied with a pointer to the following instead:
 *
 *  {
 *      {
 *          .name = NULL,
 *          .type = &aushape_conv_unique_coll_type,
 *          .args = {.unique = false}
 *       }
 *  }
 *
 * Collector-specific return codes:
 *
 * aushape_conv_coll_create:
 * aushape_conv_coll_add:
 * aushape_conv_coll_end:
 *      Any of the return codes specified collectors can return for the
 *      corresponding calls.
 */
extern const struct aushape_conv_coll_type aushape_conv_disp_coll_type;

#endif /* _AUSHAPE_CONV_DISP_COLL_H */
