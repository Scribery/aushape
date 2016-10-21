/**
 * @brief Aushape abstract output.
 *
 * Abstract output interface is used to create and manipulate specific
 * output types.
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

#ifndef _AUSHAPE_OUTPUT_H
#define _AUSHAPE_OUTPUT_H

#include <assert.h>
#include <aushape/output_type.h>

/** Abstract output instance */
struct aushape_output {
    /** Output type */
    const struct aushape_output_type   *type;
};

/**
 * Allocate and initialize an output of the specified type, with specified
 * arguments. See the particular type description for specific arguments
 * required.
 *
 * @param poutput   Location for the created output pointer, will not be
 *                  modified in case of error.
 * @param type      The type of output to create.
 * @param ...       The type-specific output creation arguments.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK                   - output created succesfully,
 *          AUSHAPE_RC_INVALID_ARGS         - invalid arguments supplied,
 *          AUSHAPE_RC_NOMEM                - failed allocating memory,
 *          AUSHAPE_RC_OUTPUT_INIT_FAILED   - output-specific initialization
 *                                            failed.
 */
extern enum aushape_rc aushape_output_create(
                                    struct aushape_output **poutput,
                                    const struct aushape_output_type *type,
                                    ...);

/**
 * Check if an output is valid.
 *
 * @param output    The output to check.
 *
 * @return True if the output is valid.
 */
extern bool aushape_output_is_valid(const struct aushape_output *output);

/**
 * Check if an output is continuous. I.e. if it can accept pieces of output
 * documents.
 *
 * @param output    The output to check. Must be valid.
 *
 * @return True if the output is continuous, false if it's discrete.
 */
static inline bool aushape_output_is_cont(
                                    const struct aushape_output *output)
{
    assert(aushape_output_is_valid(output));
    return output->type->cont;
}

/**
 * Write to an output.
 *
 * @param output    The output to write to.
 * @param ptr       Pointer to the output fragment.
 * @param len       Length of the output fragment in bytes.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK                   - written succesfully,
 *          AUSHAPE_RC_INVALID_ARGS         - invalid arguments supplied,
 *          AUSHAPE_RC_OUTPUT_WRITE_FAILED  - output-specific write failure.
 */
extern enum aushape_rc aushape_output_write(struct aushape_output *output,
                                            const char *ptr, size_t len);

/**
 * Cleanup and deallocate an output.
 *
 * @param output    The output to destroy. Must be valid, or NULL.
 */
extern void aushape_output_destroy(struct aushape_output *output);


#endif /* _AUSHAPE_OUTPUT_H */
