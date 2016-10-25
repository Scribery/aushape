/*
 * Miscellaneous syslog-related functions.
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

#include <aushape/syslog_misc.h>
#include <stdlib.h>
#include <string.h>
#define SYSLOG_NAMES
#include <syslog.h>

static int
aushape_syslog_code_from_str(CODE *list, const char *str)
{
    CODE *item;

    for (item = list; item->c_name != NULL; item++) {
        if (strcasecmp(str, item->c_name) == 0) {
            return item->c_val;
        }
    }
    return -1;
}

int
aushape_syslog_facility_from_str(const char *str)
{
    return aushape_syslog_code_from_str(facilitynames, str);
}

int
aushape_syslog_priority_from_str(const char *str)
{
    return aushape_syslog_code_from_str(prioritynames, str);
}
