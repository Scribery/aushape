/**
 * @brief A record collector type
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

#ifndef _AUSHAPE_COLL_TYPE_H
#define _AUSHAPE_COLL_TYPE_H

#include <aushape/rc.h>
#include <auparse.h>
#include <stdlib.h>
#include <stdbool.h>

/** Forward declaration of record collector instance */
struct aushape_coll;

/**
 * Collector initialization function prototype.
 *
 * @param coll      The collector to initialize, must be cleared to zero,
 *                  and have the base abstract instance initialized.
 *                  Assumed to be valid on return.
 * @param args      Initialization arguments, type-specific.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK           - initialized successfully,
 *          AUSHAPE_RC_INVALID_ARGS - invalid arguments supplied,
 *          AUSHAPE_RC_NOMEM        - memory allocation failed,
 *          other                   - collector-specific return code,
 *                                    see corresponding collector type
 *                                    documentation.
 */
typedef enum aushape_rc (*aushape_coll_type_init_fn)(
                                struct aushape_coll *coll,
                                const void *params);

/**
 * Collector validation function prototype. Called e.g. to verify arguments.
 *
 * @param coll      The collector to check. Cannot be NULL.
 *
 * @return True if the collector is valid, false otherwise.
 */
typedef bool (*aushape_coll_type_is_valid_fn)(
                                const struct aushape_coll *coll);

/**
 * Collector cleanup function prototype.
 *
 * @param coll      The collector to cleanup. Cannot be NULL, must be valid.
 */
typedef void (*aushape_coll_type_cleanup_fn)(
                                struct aushape_coll *coll);

/**
 * Prototype for a function checking if a collector is empty.
 *
 * @param coll      The collector to check. Must be valid.
 *
 * @return True if the collector is empty, false otherwise.
 */
typedef bool (*aushape_coll_type_is_empty_fn)(
                                const struct aushape_coll *coll);

/**
 * Prototype for a function emptying a collector and preparing it for
 * collection of another record sequence.
 *
 * @param coll      The collector to empty.
 */
typedef void (*aushape_coll_type_empty_fn)(
                                struct aushape_coll *coll);

/**
 * Prototype for a function adding a record to the collector record sequence.
 * Cannot be called after the sequence collection was ended. Can output a
 * complete record.
 *
 * @param coll      The collector to add the record to.
 * @param pcount    Location of/for the record counter in the container.
 *                  Incremented for every record added by the function.
 * @param level     Syntactic nesting level to output at.
 * @param prio      Growing buffer tree priority to add the record with.
 * @param au        The auparse state with the current record as the record to
 *                  be added.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK               - added successfully,
 *          AUSHAPE_RC_NOMEM            - memory allocation failed,
 *          AUSHAPE_RC_AUPARSE_FAILED   - an auparse call failed,
 *          other                       - collector-specific return code,
 *                                        see corresponding collector type
 *                                        documentation.
 */
typedef enum aushape_rc (*aushape_coll_type_add_fn)(
                                struct aushape_coll *coll,
                                size_t *pcount,
                                size_t level,
                                size_t prio,
                                auparse_state_t *au);

/**
 * Prototype for a function ending collection of the record sequence. Cannot
 * be called if the collector is empty, or after the sequence collection was
 * ended. Can output a complete record.
 *
 * @param coll      The collector to end the sequence for.
 * @param pcount    Location of/for the record counter in the container.
 *                  Incremented for every record added by the function.
 * @param level     Syntactic nesting level to output at.
 * @param prio      Growing buffer tree priority to add the record with.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK               - added successfully,
 *          AUSHAPE_RC_NOMEM            - memory allocation failed,
 *          other                       - collector-specific return code,
 *                                        see corresponding collector type
 *                                        documentation.
 */
typedef enum aushape_rc (*aushape_coll_type_end_fn)(
                                struct aushape_coll *coll,
                                size_t *pcount,
                                size_t level,
                                size_t prio);

/** Record collector type */
struct aushape_coll_type {
    /** Instance size */
    size_t  size;
    /** Initialization function */
    aushape_coll_type_init_fn       init;
    /** Validation function */
    aushape_coll_type_is_valid_fn   is_valid;
    /** Cleanup function */
    aushape_coll_type_cleanup_fn    cleanup;
    /** Emptiness-checking function */
    aushape_coll_type_is_empty_fn   is_empty;
    /** Emptying function */
    aushape_coll_type_empty_fn      empty;
    /** Record-addition function */
    aushape_coll_type_add_fn        add;
    /** Sequence-ending function */
    aushape_coll_type_end_fn        end;
};

/**
 * Check if a collector type is valid.
 *
 * @param type  The collector type to check.
 *
 * @return True if the collector type is valid, false otherwise.
 */
static inline bool
aushape_coll_type_is_valid(const struct aushape_coll_type *type)
{
    return type != NULL &&
           type->size >= sizeof(type) &&
           type->is_empty != NULL &&
           type->empty != NULL &&
           type->add != NULL;
}
#endif /* _AUSHAPE_COLL_TYPE_H */
