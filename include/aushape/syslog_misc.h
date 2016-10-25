/**
 * @brief Miscellaneous syslog-related functions.
 */
/*
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

#ifndef _AUSHAPE_SYSLOG_MISC_H
#define _AUSHAPE_SYSLOG_MISC_H

/**
 * Convert syslog facility string to integer value.
 *
 * @param str   The string to convert.
 *
 * @return The facility value, which can be supplied to openlog(3) and
 *         syslog(3), or a negative number, if the string was unrecognized.
 */
extern int aushape_syslog_facility_from_str(const char *str);

/**
 * Convert syslog priority string to integer value.
 *
 * @param str   The string to convert.
 *
 * @return The priority value, which can be supplied to openlog(3) and
 *         syslog(3), or a negative number, if the string was unrecognized.
 */
extern int aushape_syslog_priority_from_str(const char *str);

#endif /* _AUSHAPE_SYSLOG_MISC_H */
