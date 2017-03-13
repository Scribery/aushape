/**
 * Record-dropping collector
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

#include <aushape/drop_coll.h>
#include <aushape/coll.h>
#include <aushape/record.h>
#include <aushape/guard.h>
#include <string.h>

struct aushape_drop_coll {
    /** Abstract base collector */
    struct aushape_coll     coll;
};

const struct aushape_coll_type aushape_drop_coll_type = {
    .size       = sizeof(struct aushape_drop_coll),
};
