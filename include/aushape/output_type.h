/**
 * @brief Aushape output type.
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

#ifndef _AUSHAPE_OUTPUT_TYPE_H
#define _AUSHAPE_OUTPUT_TYPE_H

#include <aushape/rc.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

/** Forward declaration of the output instance */
struct aushape_output;

/**
 * Output initialization function prototype.
 *
 * @param output    The output to initialize, must be cleared to zero.
 * @param ap        Argument list.
 *
 * @return Global return code.
 */
typedef enum aushape_rc (*aushape_output_type_init_fn)(
                                struct aushape_output *output,
                                va_list ap);

/**
 * Output validation function prototype. Called e.g. to verify arguments.
 *
 * @param output    The output to check.
 *
 * @return True if the output is valid, false otherwise.
 */
typedef bool (*aushape_output_type_is_valid_fn)(
                                const struct aushape_output *output);

/**
 * Output writing function prototype. Called for each output fragment.
 *
 * @param output    The output to write to.
 * @param ptr       Pointer to the output fragment.
 * @param len       Length of the output fragment in bytes.
 *
 * @return Return code.
 *         See specific type descriptions for particular return codes.
 */
typedef enum aushape_rc (*aushape_output_type_write_fn)(
                                struct aushape_output *output,
                                const char *ptr, size_t len);

/**
 * Output cleanup function prototype.
 *
 * @param output    The output to cleanup.
 */
typedef void (*aushape_output_type_cleanup_fn)(struct aushape_output *output);

/** Output type */
struct aushape_output_type {
    /** Instance size */
    size_t  size;
    /**
     * True if the output accepts continuous output, i.e. arbitrary pieces of
     * the output, false if it expects complete documents/events.
     */
    bool    cont;
    /** Initialization function */
    aushape_output_type_init_fn     init;
    /** Validation function */
    aushape_output_type_is_valid_fn is_valid;
    /** Write function */
    aushape_output_type_write_fn    write;
    /** Cleanup function */
    aushape_output_type_cleanup_fn  cleanup;
};

/**
 * Check if an output type is valid.
 *
 * @param type  The type to check.
 *
 * @return True if the output type is valid, false otherwise.
 */
static inline bool
aushape_output_type_is_valid(const struct aushape_output_type *type)
{
    return type != NULL &&
           type->size >= sizeof(type) &&
           type->write != NULL;
}

#endif /* _AUSHAPE_OUTPUT_TYPE_H */
