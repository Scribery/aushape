/**
 * @brief Aushape audit log record manipulation
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

#ifndef _AUSHAPE_RECORD_H
#define _AUSHAPE_RECORD_H

#include <aushape/gbuf.h>
#include <aushape/format.h>
#include <aushape/rc.h>
#include <auparse.h>

/**
 * Output auparse record fields to a growing buffer according to a format and
 * syntactic nesting level.
 *
 * @param gbuf      The growing buffer to add the formatted record to.
 * @param format    The output format to use.
 * @param level     Syntactic nesting level the fields are output at.
 * @param au        The auparse state with the current record as the one to
 *                  output fields of.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK               - output succesfully,
 *          AUSHAPE_RC_INVALID_ARGS     - invalid arguments supplied,
 *          AUSHAPE_RC_NOMEM            - memory allocation failed,
 *          AUSHAPE_RC_AUPARSE_FAILED   - an auparse call failed.
 */
extern enum aushape_rc aushape_record_format_fields(
                                    struct aushape_gbuf *gbuf,
                                    const struct aushape_format *format,
                                    size_t level,
                                    auparse_state_t *au);

/**
 * Output an auparse record to a growing buffer according to format and
 * syntactic nesting level.
 *
 * @param gbuf      The growing buffer to add the formatted record to.
 * @param format    The output format to use.
 * @param level     Syntactic nesting level the record is output at.
 * @param first     True if this is the first record being output for a
 *                  container, false otherwise.
 * @param name      The record container name.
 * @param au        The auparse state with the current record as the one to be
 *                  output.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK               - output succesfully,
 *          AUSHAPE_RC_INVALID_ARGS     - invalid arguments supplied,
 *          AUSHAPE_RC_NOMEM            - memory allocation failed,
 *          AUSHAPE_RC_AUPARSE_FAILED   - an auparse call failed.
 */
extern enum aushape_rc aushape_record_format(
                                    struct aushape_gbuf *gbuf,
                                    const struct aushape_format *format,
                                    size_t level,
                                    bool first,
                                    const char *name,
                                    auparse_state_t *au);

#endif /* _AUSHAPE_RECORD_H */
