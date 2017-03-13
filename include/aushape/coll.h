/**
 * @brief Abstract record collector interface
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

#ifndef _AUSHAPE_COLL_H
#define _AUSHAPE_COLL_H

#include <aushape/coll_type.h>
#include <aushape/gbtree.h>

/** Abstract record collector instance */
struct aushape_coll {
    /** Collector type */
    const struct aushape_coll_type *type;
    /** The output format to use */
    struct aushape_format           format;
    /** The growing buffer tree to add collected output records to */
    struct aushape_gbtree          *gbtree;
    /** True if the record sequence was ended, false otherwise */
    bool                            ended;
};

/**
 * Create (allocate and initialize) a collector.
 *
 * @param pcoll     Location for the created collector pointer, cannot be NULL.
 * @param type      The type of the collector to create.
 * @param format    The output format to use.
 * @param gbtree    The growing buffer tree to add collected output records
 *                  to. Never emptied by the collector. Must stay valid for
 *                  the existence of the collector.
 * @param args      Initialization arguments, type-specific. See description
 *                  of the corresponding type for the expected values.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK               - initialized successfully,
 *          AUSHAPE_RC_INVALID_ARGS     - invalid arguments supplied,
 *          AUSHAPE_RC_NOMEM            - memory allocation failed,
 *          other                       - collector-specific return code,
 *                                        see corresponding collector type
 *                                        documentation.
 */
extern enum aushape_rc aushape_coll_create(
                                struct aushape_coll **pcoll,
                                const struct aushape_coll_type *type,
                                const struct aushape_format *format,
                                struct aushape_gbtree *gbtree,
                                const void *args);

/**
 * Check if a collector is valid. Called e.g. to verify arguments.
 *
 * @param coll      The collector to check.
 *
 * @return True if the collector is valid, false otherwise.
 */
extern bool aushape_coll_is_valid(const struct aushape_coll *coll);

/**
 * Destroy (cleanup and free) a collector.
 *
 * @param coll      The collector to destroy, can be NULL,
 *                  otherwise must be valid.
 */
extern void aushape_coll_destroy(struct aushape_coll *coll);

/**
 * Check if a collector is empty, i.e. if it has not accumulated information
 * on a record sequence that can be ended.
 *
 * @param coll      The collector to check, must be valid.
 *
 * @return True if the collector is empty, false otherwise.
 */
extern bool aushape_coll_is_empty(const struct aushape_coll *coll);

/**
 * Empty a collector and prepare it for collection of another record sequence.
 *
 * @param coll      The collector to empty.
 */
extern void aushape_coll_empty(struct aushape_coll *coll);

/**
 * Check if a collector record sequence was ended.
 *
 * @param coll      The collector to check, must be valid.
 *
 * @return True if the collector record sequence was ended, false otherwise.
 */
extern bool aushape_coll_is_ended(const struct aushape_coll *coll);

/**
 * Add a record to the collector record sequence. Can output a complete
 * record. Cannot be called after the sequence collection was ended, without
 * emptying the collector first.
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
 *          AUSHAPE_RC_INVALID_ARGS     - invalid arguments supplied,
 *          AUSHAPE_RC_INVALID_STATE    - called after ending record
 *                                        sequence, without emptying,
 *          AUSHAPE_RC_NOMEM            - memory allocation failed,
 *          AUSHAPE_RC_AUPARSE_FAILED   - an auparse call failed,
 *          other                       - collector-specific errors,
 *                                        see corresponding collector type
 *                                        documentation.
 */
extern enum aushape_rc aushape_coll_add(struct aushape_coll *coll,
                                        size_t *pcount,
                                        size_t level,
                                        size_t prio,
                                        auparse_state_t *au);

/**
 * End collection of a collector's record sequence. Can output a complete
 * record. Has no effect if no record sequence has been accumulated by the
 * collector. Cannot be called repeatedly, without emptying the collector.
 *
 * @param coll      The collector to end the sequence for.
 * @param pcount    Location of/for the record counter in the container.
 *                  Incremented for every record added by the function.
 * @param level     Syntactic nesting level to output at.
 * @param prio      Growing buffer tree priority to add the record with.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK               - added successfully,
 *          AUSHAPE_RC_INVALID_ARGS     - invalid arguments supplied,
 *          AUSHAPE_RC_INVALID_STATE    - called after ending record
 *                                        sequence, without emptying,
 *          AUSHAPE_RC_NOMEM            - memory allocation failed,
 *          other                       - collector-specific errors,
 *                                        see corresponding collector type
 *                                        documentation.
 */
extern enum aushape_rc aushape_coll_end(struct aushape_coll *coll,
                                        size_t *pcount,
                                        size_t level,
                                        size_t prio);

#endif /* _AUSHAPE_COLL_H */
