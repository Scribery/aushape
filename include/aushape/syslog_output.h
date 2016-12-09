/**
 * @file
 * @brief Syslog discrete aushape output.
 *
 * An implementation of an output writing discrete output fragments to syslog.
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

#ifndef _AUSHAPE_SYSLOG_OUTPUT_H
#define _AUSHAPE_SYSLOG_OUTPUT_H

#include <aushape/output.h>

/** Syslog output type */
extern const struct aushape_output_type aushape_syslog_output_type;

/**
 * Create an instance of syslog output.
 *
 * @param poutput   Location for the created output pointer, will be set to
 *                  NULL in case of error.
 * @param priority  The "priority" argument to pass to syslog(3).
 *
 * @return Return code:
 *          AUSHAPE_RC_OK           - output created successfully,
 *          AUSHAPE_RC_INVALID_ARGS - invalid arguments supplied,
 *          AUSHAPE_RC_NOMEM        - failed allocating memory.
 */
static inline enum aushape_rc
aushape_syslog_output_create(struct aushape_output **poutput,
                             int priority)
{
    return aushape_output_create(poutput, &aushape_syslog_output_type,
                                 priority);
}

#endif /* _AUSHAPE_SYSLOG_OUTPUT_H */
