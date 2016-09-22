/**
 * @brief Aushape output format
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

#ifndef _AUSHAPE_FORMAT_H
#define _AUSHAPE_FORMAT_H

#include <stdbool.h>
#include <assert.h>

/** Output format */
enum aushape_format {
    AUSHAPE_FORMAT_INVALID, /** Invalid, uninitialized format */
    AUSHAPE_FORMAT_XML,     /** XML */
    AUSHAPE_FORMAT_JSON,    /** JSON */
    AUSHAPE_FORMAT_NUM      /** Number of formats (not a valid format) */
};

/**
 * Check if a format is valid.
 *
 * @param format    The format to check.
 *
 * @return True if the format is valid, false otherwise.
 */
bool
aushape_format_is_valid(enum aushape_format format)
{
    return format > AUSHAPE_FORMAT_INVALID &&
           format < AUSHAPE_FORMAT_NUM;
}

#endif /* _AUSHAPE_FORMAT_H */
