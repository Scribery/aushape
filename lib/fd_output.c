/**
 * File descriptor aushape output.
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

#include <aushape/fd_output.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

/** FD output data */
struct aushape_fd_output {
    struct aushape_output output;   /**< Abstract output instance */
    int fd;                         /**< FD to write to */
    bool fd_owned;                  /**< True if FD is owned */
};

static enum aushape_rc
aushape_fd_output_init(struct aushape_output *output, va_list ap)
{
    struct aushape_fd_output *fd_output = (struct aushape_fd_output*)output;
    int fd = va_arg(ap, int);
    bool fd_owned = (bool)va_arg(ap, int);

    assert(fd_output != NULL);

    if (fd < 0) {
        return AUSHAPE_RC_INVALID_ARGS;
    }

    fd_output->fd = fd;
    fd_output->fd_owned = fd_owned;

    return AUSHAPE_RC_OK;
}

static bool
aushape_fd_output_is_valid(const struct aushape_output *output)
{
    struct aushape_fd_output *fd_output = (struct aushape_fd_output*)output;
    assert(fd_output != NULL);

    return fd_output->fd >= 0;
}

static void
aushape_fd_output_cleanup(struct aushape_output *output)
{
    struct aushape_fd_output *fd_output = (struct aushape_fd_output*)output;
    assert(fd_output != NULL);

    if (fd_output->fd_owned) {
        close(fd_output->fd);
        fd_output->fd = -1;
        fd_output->fd_owned = false;
    }
}

static enum aushape_rc
aushape_fd_output_write(struct aushape_output *output,
                        const char *ptr,
                        size_t len)
{
    struct aushape_fd_output *fd_output = (struct aushape_fd_output*)output;
    ssize_t rc;

    assert(fd_output != NULL);

    while (true) {
        rc = write(fd_output->fd, ptr, len);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                return AUSHAPE_RC_OUTPUT_WRITE_FAILED;
            }
        }
        if ((size_t)rc == len) {
            return AUSHAPE_RC_OK;
        }
        ptr += rc;
        len -= (size_t)rc;
    }
}

const struct aushape_output_type aushape_fd_output_type = {
    .size       = sizeof(struct aushape_fd_output),
    .cont       = true,
    .init       = aushape_fd_output_init,
    .is_valid   = aushape_fd_output_is_valid,
    .write      = aushape_fd_output_write,
    .cleanup    = aushape_fd_output_cleanup,
};
