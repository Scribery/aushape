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

/**
 * Dispatching record collector type.
 *
 * Collector-specific return codes:
 *
 * aushape_conv_coll_add:
 *      AUSHAPE_RC_CONV_INVALID_EXECVE      - invalid execve record sequence
 *                                            encountered
 *      AUSHAPE_RC_CONV_REPEATED_RECORD     - an unexpected repeated record
 *                                            type encountered
 */
extern const struct aushape_conv_coll_type aushape_conv_disp_coll_type;

#endif /* _AUSHAPE_CONV_DISP_COLL_H */
