/*
 * (Exponentially) growing array.
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

#include <aushape/garr.h>
#include <aushape/guard.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

bool
aushape_garr_is_valid(const struct aushape_garr *garr)
{
    return garr != NULL &&
           garr->item_size != 0 &&
           garr->init_alloc_len != 0 &&
           (garr->alloc_len == 0 ||
            (garr->ptr != NULL && garr->alloc_len >= garr->init_alloc_len)) &&
           garr->valid_len <= garr->alloc_len;
}

void
aushape_garr_init(struct aushape_garr *garr,
                  size_t item_size,
                  size_t alloc_len)
{
    assert(garr != NULL);
    assert(item_size != 0);
    assert(alloc_len != 0);
    memset(garr, 0, sizeof(*garr));
    garr->item_size = item_size;
    garr->init_alloc_len = alloc_len;
    assert(aushape_garr_is_valid(garr));
}

void
aushape_garr_cleanup(struct aushape_garr *garr)
{
    assert(aushape_garr_is_valid(garr));
    free(garr->ptr);
    memset(garr, 0, sizeof(*garr));
}

void
aushape_garr_empty(struct aushape_garr *garr)
{
    assert(aushape_garr_is_valid(garr));
    garr->valid_len = 0;
}

bool
aushape_garr_is_empty(const struct aushape_garr *garr)
{
    assert(aushape_garr_is_valid(garr));
    return garr->valid_len == 0;
}

enum aushape_rc
aushape_garr_accomodate(struct aushape_garr *garr, size_t len)
{
    enum aushape_rc rc;
    assert(aushape_garr_is_valid(garr));

    if (len > garr->alloc_len) {
        char *new_ptr;
        size_t new_alloc_len = garr->alloc_len == 0
                                    ? garr->init_alloc_len
                                    : garr->alloc_len * 2;
        while (new_alloc_len < len) {
            new_alloc_len *= 2;
        }
        new_ptr = realloc(garr->ptr, garr->item_size * new_alloc_len);
        AUSHAPE_GUARD_BOOL(NOMEM, new_ptr != NULL);
        garr->ptr = new_ptr;
        garr->alloc_len = new_alloc_len;
    }

    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

enum aushape_rc
aushape_garr_set(struct aushape_garr *garr, size_t index, const void *item)
{
    enum aushape_rc rc;
    size_t end_len = index + 1;
    assert(aushape_garr_is_valid(garr));
    assert(item != NULL);
    if (end_len > garr->valid_len) {
        AUSHAPE_GUARD(aushape_garr_accomodate(garr, end_len));
        garr->valid_len = end_len;
    }
    memcpy((char *)garr->ptr + index * garr->item_size,
           item, garr->item_size);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

enum aushape_rc
aushape_garr_set_span(struct aushape_garr *garr, size_t index,
                      const void *item, size_t len)
{
    enum aushape_rc rc;
    size_t end_len = index + len;
    assert(aushape_garr_is_valid(garr));
    assert(item != NULL || len == 0);
    if (end_len > garr->valid_len) {
        AUSHAPE_GUARD(aushape_garr_accomodate(garr, end_len));
        garr->valid_len = end_len;
    }
    for (; index < end_len; index++) {
        memcpy((char *)garr->ptr + index * garr->item_size,
               item, garr->item_size);
    }
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

enum aushape_rc
aushape_garr_set_byte_span(struct aushape_garr *garr,
                           size_t index,
                           unsigned char byte,
                           size_t len)
{
    enum aushape_rc rc;
    size_t end_len = index + len;
    assert(aushape_garr_is_valid(garr));
    if (end_len > garr->valid_len) {
        AUSHAPE_GUARD(aushape_garr_accomodate(garr, end_len));
        garr->valid_len = end_len;
    }
    memset((char *)garr->ptr + index * garr->item_size,
           byte, len * garr->item_size);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

enum aushape_rc
aushape_garr_set_arr(struct aushape_garr *garr,
                     size_t index,
                     const void *ptr,
                     size_t len)
{
    enum aushape_rc rc;
    size_t end_len = index + len;
    assert(aushape_garr_is_valid(garr));
    assert(ptr != NULL || len == 0);
    if (end_len > garr->valid_len) {
        AUSHAPE_GUARD(aushape_garr_accomodate(garr, end_len));
        garr->valid_len = end_len;
    }
    memcpy(garr->ptr + index * garr->item_size,
           ptr, len * garr->item_size);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

