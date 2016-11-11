/*
 * Aushape function return codes
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

#include <aushape/rc.h>
#include <aushape/misc.h>
#include <stddef.h>

struct aushape_rc_info {
    const char *name;
    const char *desc;
};

static const struct aushape_rc_info \
                        aushape_rc_info_list[AUSHAPE_RC_NUM] = {
#define RC(_name, _desc) \
    [AUSHAPE_RC_##_name] = {.name = #_name, .desc = _desc}
    RC(OK,
       "Success"),
    RC(INVALID_ARGS,
       "Invalid arguments supplied to a function"),
    RC(INVALID_STATE,
       "Object is in invalid state"),
    RC(NOMEM,
       "Memory allocation failed"),
    RC(AUPARSE_FAILED,
       "A call to auparse failed"),
    RC(INVALID_EXECVE,
       "Invalid execve record sequence encountered"),
    RC(REPEATED_RECORD,
       "An unexpected repeated record type encountered"),
    RC(OUTPUT_INIT_FAILED,
       "Output initialization failed"),
    RC(OUTPUT_WRITE_FAILED,
       "Output write failed"),
#undef RC
};

const char *
aushape_rc_to_name(enum aushape_rc rc)
{
    if ((size_t)rc < AUSHAPE_ARRAY_SIZE(aushape_rc_info_list)) {
        const struct aushape_rc_info *info = aushape_rc_info_list + rc;
        if (info->name != NULL) {
            return info->name;
        }
    }
    return "UNKNOWN";
}

const char *
aushape_rc_to_desc(enum aushape_rc rc)
{
    if ((size_t)rc < AUSHAPE_ARRAY_SIZE(aushape_rc_info_list)) {
        const struct aushape_rc_info *info = aushape_rc_info_list + rc;
        if (info->desc != NULL) {
            return info->desc;
        }
    }
    return "Unknown return code";
}
