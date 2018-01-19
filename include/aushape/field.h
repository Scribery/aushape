/**
 * @brief Aushape audit log record field manipulation
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

#ifndef _AUSHAPE_FIELD_H
#define _AUSHAPE_FIELD_H

#include <aushape/gbuf.h>
#include <aushape/format.h>
#include <aushape/rc.h>
#include <auparse.h>

/**
 * Output an auparse field as extracted properties to a growing buffer
 * according to format and syntactic nesting level.
 *
 * @param gbuf      The growing buffer to add the formatted field to.
 * @param format    The output format to use.
 * @param level     Syntactic nesting level the field is output at.
 * @param first     True if this is the first field being output for a
 *                  container, false otherwise.
 * @param list      True if outputting a list item,
 *                  false if outputting a "map" item.
 * @param name      The field "element" name. Not used for list items in
 *                  languages where they don't have to be named, such as JSON.
 * @param value_r   The raw field value, NULL for omitted.
 * @param value_i   The "interpreted" field value, NULL for omitted.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK               - output successfully,
 *          AUSHAPE_RC_INVALID_ARGS     - invalid arguments supplied,
 *          AUSHAPE_RC_NOMEM            - memory allocation failed,
 */
extern enum aushape_rc aushape_field_format_props(
                                    struct aushape_gbuf *gbuf,
                                    const struct aushape_format *format,
                                    size_t level,
                                    bool first,
                                    bool list,
                                    const char *name,
                                    const char *value_r,
                                    const char *value_i);

/**
 * Output an auparse field to a growing buffer according to format and
 * syntactic nesting level.
 *
 * @param gbuf      The growing buffer to add the formatted field to.
 * @param format    The output format to use.
 * @param level     Syntactic nesting level the field is output at.
 * @param first     True if this is the first field being output for a
 *                  container, false otherwise.
 * @param list      True if outputting a list item,
 *                  false if outputting a "map" item.
 * @param name      The field "element" name. Not used for list items in
 *                  languages where they don't have to be named, such as JSON.
 * @param au        The auparse state with the current field as the one to be
 *                  output.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK               - output successfully,
 *          AUSHAPE_RC_INVALID_ARGS     - invalid arguments supplied,
 *          AUSHAPE_RC_NOMEM            - memory allocation failed,
 *          AUSHAPE_RC_AUPARSE_FAILED   - an auparse call failed.
 */
extern enum aushape_rc aushape_field_format(
                                    struct aushape_gbuf *gbuf,
                                    const struct aushape_format *format,
                                    size_t level,
                                    bool first,
                                    bool list,
                                    const char *name,
                                    auparse_state_t *au);

#endif /* _AUSHAPE_FIELD_H */
