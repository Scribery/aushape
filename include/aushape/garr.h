/**
 * @brief (Exponentially) growing array
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

#ifndef _AUSHAPE_GARR_H
#define _AUSHAPE_GARR_H

#include <aushape/rc.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

/** An (exponentially) growing array */
struct aushape_garr {
    void   *ptr;            /**< Pointer to the buffer memory */
    size_t  item_size;      /**< Item size */
    size_t  valid_len;      /**< Valid item number */
    size_t  alloc_len;      /**< Allocated item number */
    size_t  init_alloc_len; /**< Initial allocated item number */
};

/**
 * Check if a growing array is valid.
 *
 * @param garr  The growing array to check.
 *
 * @return True if the growing array is valid, false otherwise.
 */
extern bool aushape_garr_is_valid(const struct aushape_garr *garr);

/**
 * Initialize a growing array.
 *
 * @param garr      The growing array to initialize.
 * @param item_size The array item size, bytes. Cannot be zero.
 * @param alloc_len Initial array length to allocate. Cannot be zero.
 */
extern void aushape_garr_init(struct aushape_garr *garr,
                              size_t item_size,
                              size_t alloc_len);

/**
 * Cleanup a growing array.
 *
 * @param garr  The growing array to cleanup.
 */
extern void aushape_garr_cleanup(struct aushape_garr *garr);

/**
 * Empty a growing array.
 *
 * @param garr  The growing array to empty.
 */
extern void aushape_garr_empty(struct aushape_garr *garr);

/**
 * Check if a growing array is empty.
 *
 * @param garr  The growing array to check.
 *
 * @return True if the buffer is empty, false otherwise.
 */
extern bool aushape_garr_is_empty(const struct aushape_garr *garr);

/**
 * Retrieve a growing array length.
 *
 * @param garr  The growing array to retrieve the length of.
 *
 * @return The array length (number of valid items).
 */
static inline size_t
aushape_garr_get_len(const struct aushape_garr *garr)
{
    assert(aushape_garr_is_valid(garr));
    return garr->valid_len;
}

/**
 * Make sure a growing array can accommodate the specified number of items.
 * If the allocated buffer memory wasn't enough to accommodate the
 * specified size, then the memory is reallocated appropriately.
 *
 * @param garr  The growing array to accommodate the contents in.
 * @param len   The size of the contents in bytes.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - accommodated successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_garr_accomodate(struct aushape_garr *garr,
                                               size_t len);

/**
 * Store an item copy in a growing array at a specified index.
 *
 * @param garr  The growing array to store the item in.
 * @param index The index to store the item at.
 * @param item  The item to store.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_garr_set(struct aushape_garr *garr,
                                        size_t index,
                                        const void *item);

/**
 * Add an item copy to the end of a growing array.
 *
 * @param garr  The growing array to add the item to.
 * @param item  The item to add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
static inline enum aushape_rc
aushape_garr_add(struct aushape_garr *garr, const void *item)
{
    assert(aushape_garr_is_valid(garr));
    assert(item != NULL);
    return aushape_garr_set(garr, garr->valid_len, item);
}

/**
 * Store a span filled with item copies in a growing array at a specified
 * index.
 *
 * @param garr  The growing array to store the span in.
 * @param index The index to store the span at.
 * @param item  The item to fill the span with.
 * @param len   The length of the span to fill.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_garr_set_span(struct aushape_garr *garr,
                                             size_t index,
                                             const void *item,
                                             size_t len);

/**
 * Add a span filled with an item to the end of a growing array.
 *
 * @param garr  The growing array to add the span to.
 * @param item  The item to fill the span with.
 * @param len   The length of the span to fill.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
static inline enum aushape_rc
aushape_garr_add_span(struct aushape_garr *garr, const void *item, size_t len)
{
    assert(aushape_garr_is_valid(garr));
    assert(item != NULL || len == 0);
    return aushape_garr_set_span(garr, garr->valid_len, item, len);
}

/**
 * Store a number of items filled with a specified byte in a growing array at
 * a specified index.
 *
 * @param garr  The growing array to store the span in.
 * @param index The index to store the items at.
 * @param byte  The byte to fill the stored items with.
 * @param len   The number of items to fill.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_garr_set_byte_span(struct aushape_garr *garr,
                                                  size_t index,
                                                  unsigned char byte,
                                                  size_t len);

/**
 * Add a number of items filled with a specified byte to the end of a growing
 * array.
 *
 * @param garr  The growing array to add the span to.
 * @param byte  The byte to fill the added items with.
 * @param len   The number of items to add and fill.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
static inline enum aushape_rc
aushape_garr_add_byte_span(struct aushape_garr *garr,
                           unsigned char byte, size_t len)
{
    assert(aushape_garr_is_valid(garr));
    return aushape_garr_set_byte_span(garr, garr->valid_len, byte, len);
}

/**
 * Store a number of items filled with zero byte in a growing array at a
 * specified index.
 *
 * @param garr  The growing array to store the span in.
 * @param index The index to store the items at.
 * @param len   The number of items to add and fill.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
static inline enum aushape_rc
aushape_garr_set_zero_span(struct aushape_garr *garr,
                           size_t index,
                           size_t len)
{
    assert(aushape_garr_is_valid(garr));
    return aushape_garr_set_byte_span(garr, index, 0, len);
}

/**
 * Add a number of items filled with zero byte to the end of a growing array.
 *
 * @param garr  The growing array to add the span to.
 * @param len   The number of items to add and fill.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
static inline enum aushape_rc
aushape_garr_add_zero_span(struct aushape_garr *garr, size_t len)
{
    assert(aushape_garr_is_valid(garr));
    return aushape_garr_add_byte_span(garr, 0, len);
}

/**
 * Store a zeroed item in a growing array at a specified index.
 *
 * @param garr  The growing array to store a zeroed item to.
 * @param index The index to store the item at.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
static inline enum aushape_rc
aushape_garr_set_zero(struct aushape_garr *garr, size_t index)
{
    assert(aushape_garr_is_valid(garr));
    return aushape_garr_set_zero_span(garr, index, 1);
}

/**
 * Add a zeroed item to the end of a growing array.
 *
 * @param garr  The growing array to add a zeroed item to.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
static inline enum aushape_rc
aushape_garr_add_zero(struct aushape_garr *garr)
{
    assert(aushape_garr_is_valid(garr));
    return aushape_garr_add_zero_span(garr, 1);
}

/**
 * Store the copy of an abstract item array in a growing array at a
 * specified index.
 *
 * @param garr  The growing array to add the abstract array to.
 * @param index The index to store the array at.
 * @param ptr   The pointer to the array to add.
 * @param len   The length of the array to add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_garr_set_arr(struct aushape_garr *garr,
                                            size_t index,
                                            const void *ptr,
                                            size_t len);

/**
 * Add the copy of an abstract item array to the end of a growing array.
 *
 * @param garr  The growing array to add the abstract array to.
 * @param ptr   The pointer to the array to add.
 * @param len   The length of the array to add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
static inline enum aushape_rc
aushape_garr_add_arr(struct aushape_garr *garr, const void *ptr, size_t len)
{
    assert(aushape_garr_is_valid(garr));
    assert(ptr != NULL || len == 0);
    return aushape_garr_set_arr(garr, garr->valid_len, ptr, len);
}

/**
 * Get an item from a specified index in a growing array.
 *
 * @param garr  The array to get the item from.
 * @param index The index in the array to get the item from, must be within
 *              array bounds.
 *
 * @return The item pointer.
 */
static inline void *
aushape_garr_get(struct aushape_garr *garr, size_t index)
{
    assert(aushape_garr_is_valid(garr));
    assert(index < garr->valid_len);
    return (char *)garr->ptr + index * garr->item_size;
}

/**
 * Get an item from a specified index in a constant growing array.
 *
 * @param garr  The array to get the item from.
 * @param index The index in the array to get the item from, must be within
 *              array bounds.
 *
 * @return The item pointer.
 */
static inline const void *
aushape_garr_const_get(const struct aushape_garr *garr, size_t index)
{
    assert(aushape_garr_is_valid(garr));
    assert(index < garr->valid_len);
    return (const char *)garr->ptr + index * garr->item_size;
}

#endif /* _AUSHAPE_GARR_H */
