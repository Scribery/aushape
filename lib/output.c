/**
 * Aushape abstract output.
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

#include <aushape/output.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

enum aushape_rc
aushape_output_create(struct aushape_output **poutput,
                      const struct aushape_output_type *type,
                      ...)
{
    va_list ap;
    enum aushape_rc rc;
    struct aushape_output *output;

    if (poutput == NULL ||
        !aushape_output_type_is_valid(type)) {
        return AUSHAPE_RC_INVALID_ARGS;
    }

    output = calloc(type->size, 1);
    if (output == NULL) {
        rc = AUSHAPE_RC_NOMEM;
    } else {
        output->type = type;

        if (type->init == NULL) {
            rc = AUSHAPE_RC_OK;
        } else {
            va_start(ap, type);
            rc = type->init(output, ap);
            va_end(ap);
        }

        if (rc == AUSHAPE_RC_OK) {
            assert(aushape_output_is_valid(output));
            *poutput = output;
        } else {
            free(output);
        }
    }

    return rc;
}

bool
aushape_output_is_valid(const struct aushape_output *output)
{
    return output != NULL &&
           aushape_output_type_is_valid(output->type) && (
               output->type->is_valid == NULL ||
               output->type->is_valid(output)
           );
}

enum aushape_rc
aushape_output_write(struct aushape_output *output,
                     const char *ptr, size_t len)
{
    if (!aushape_output_is_valid(output) ||
        (ptr == NULL && len != 0)) {
        return AUSHAPE_RC_INVALID_ARGS;
    }
    return output->type->write(output, ptr, len);
}

void
aushape_output_destroy(struct aushape_output *output)
{
    assert(output == NULL || aushape_output_is_valid(output));

    if (output == NULL) {
        return;
    }
    if (output->type->cleanup != NULL) {
        output->type->cleanup(output);
    }
    memset(output, 0, output->type->size);
    free(output);
}
