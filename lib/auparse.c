/**
 * Substitutes for possibly missing auparse functions.
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

#include <config.h>
#include <aushape/auparse.h>
#ifndef HAVE_AUPARSE_GET_TYPE_NAME
#include <libaudit.h>
#endif
#include <assert.h>

const char *
aushape_auparse_get_type_name(auparse_state_t *au)
{
    assert(au != NULL);
#ifdef HAVE_AUPARSE_GET_TYPE_NAME
    return auparse_get_type_name(au);
#else
    int type = auparse_get_type(au);
    return (type == 0) ? NULL : audit_msg_type_to_name(type);
#endif
}
