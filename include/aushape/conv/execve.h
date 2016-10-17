/**
 * @brief An execve record aggregation buffer for raw audit log converter
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

#ifndef _AUSHAPE_CONV_EXECVE_H
#define _AUSHAPE_CONV_EXECVE_H

#include <aushape/format.h>
#include <aushape/gbuf.h>
#include <aushape/conv/rc.h>
#include <auparse.h>

/** Execve record aggregation buffer */
struct aushape_conv_execve {
    /** Formatted raw log buffer */
    struct aushape_gbuf raw;
    /** Formatted argument list buffer */
    struct aushape_gbuf args;
    /** Number of arguments expected */
    size_t              arg_num;
    /** Index of the argument being read */
    size_t              arg_idx;
    /** True if argument length is specified */
    bool                got_len;
    /** Index of the argument slice being read */
    size_t              slice_idx;
    /** Total length of the argument being read */
    size_t              len_total;
    /** Length of the argument read so far */
    size_t              len_read;
};

/**
 * Check if an execve record aggregation buffer is valid.
 *
 * @param execve    The buffer to check.
 *
 * @return True if the buffer is valid, false otherwise.
 */
extern bool aushape_conv_execve_is_valid(
                        const struct aushape_conv_execve *execve);

/**
 * Initialize an execve record aggregation buffer.
 *
 * @param execve    The buffer to initialize.
 */
extern void aushape_conv_execve_init(struct aushape_conv_execve *execve);

/**
 * Cleanup an execve record aggregation buffer.
 *
 * @param execve    The buffer to cleanup, must be valid.
 */
extern void aushape_conv_execve_cleanup(struct aushape_conv_execve *execve);

/**
 * Empty an execve record aggregation buffer to prepare for processing of
 * another execve record sequence.
 *
 * @param execve    The buffer to empty.
 */
extern void aushape_conv_execve_empty(struct aushape_conv_execve *execve);

/**
 * Aggregate an execve record data into a buffer.
 *
 * @param execve    The buffer to aggregate the record into.
 * @param format    The output format to use.
 * @param au        The auparse state with the current record as the execve
 *                  record to be aggregated.
 *
 * @return Return code:
 *          AUSHAPE_CONV_RC_OK              - added succesfully,
 *          AUSHAPE_CONV_RC_NOMEM           - memory allocation failed,
 *          AUSHAPE_CONV_RC_AUPARSE_FAILED  - an auparse call failed,
 *          AUSHAPE_CONV_RC_INVALID_EXECVE  - invalid execve record sequence
 *                                            encountered.
 */
extern enum aushape_conv_rc aushape_conv_execve_add(
                                    struct aushape_conv_execve *execve,
                                    const struct aushape_format *format,
                                    auparse_state_t *au);

/**
 * Check if an execve aggregation buffer is complete.
 *
 * @param execve    The buffer to check.
 *
 * @return True if the execve aggregation buffer is complete, false otherwise.
 */
static inline bool
aushape_conv_execve_is_complete(const struct aushape_conv_execve *execve)
{
    assert(aushape_conv_execve_is_valid(execve));
    return execve->arg_idx == execve->arg_num;
}

#endif /* _AUSHAPE_CONV_EXECVE_H */
