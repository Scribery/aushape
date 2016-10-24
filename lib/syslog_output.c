/**
 * Syslog discrete aushape output.
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

#include <aushape/syslog_output.h>
#include <assert.h>
#include <limits.h>
#include <syslog.h>

/** Syslog output data */
struct aushape_syslog_output {
    struct aushape_output output;   /**< Abstract output instance */
    int                   priority; /**< Syslog(3) priority */
};

static enum aushape_rc
aushape_syslog_output_init(struct aushape_output *output, va_list ap)
{
    struct aushape_syslog_output *syslog_output =
                                    (struct aushape_syslog_output*)output;
    assert(syslog_output != NULL);
    syslog_output->priority = va_arg(ap, int);
    return AUSHAPE_RC_OK;
}

static enum aushape_rc
aushape_syslog_output_write(struct aushape_output *output,
                        const char *ptr,
                        size_t len)
{
    struct aushape_syslog_output *syslog_output = (struct aushape_syslog_output*)output;

    assert(syslog_output != NULL);

    if (len > INT_MAX) {
        return AUSHAPE_RC_INVALID_ARGS;
    }

    syslog(syslog_output->priority, "%.*s",
           (int)len, (const char *)ptr);

    return AUSHAPE_RC_OK;
}

const struct aushape_output_type aushape_syslog_output_type = {
    .size       = sizeof(struct aushape_syslog_output),
    .cont       = false,
    .init       = aushape_syslog_output_init,
    .write      = aushape_syslog_output_write,
};
