/**
 * Abstract record collector interface
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

#include <aushape/coll.h>
#include <string.h>

enum aushape_rc
aushape_coll_create(struct aushape_coll **pcoll,
                    const struct aushape_coll_type *type,
                    const struct aushape_format *format,
                    struct aushape_gbtree *gbtree,
                    const void *args)
{
    enum aushape_rc rc;
    struct aushape_coll *coll;

    if (pcoll == NULL ||
        !aushape_coll_type_is_valid(type) ||
        !aushape_format_is_valid(format) ||
        !aushape_gbtree_is_valid(gbtree)) {
        return AUSHAPE_RC_INVALID_ARGS;
    }

    coll = calloc(type->size, 1);
    if (coll == NULL) {
        rc = AUSHAPE_RC_NOMEM;
    } else {
        coll->type = type;
        coll->format = *format;
        coll->gbtree = gbtree;

        rc = (type->init != NULL) ? type->init(coll, args) : AUSHAPE_RC_OK;
        if (rc == AUSHAPE_RC_OK) {
            assert(aushape_coll_is_valid(coll));
            *pcoll = coll;
        } else {
            free(coll);
        }
    }

    return rc;
}

bool
aushape_coll_is_valid(const struct aushape_coll *coll)
{
    return coll != NULL &&
           aushape_coll_type_is_valid(coll->type) &&
           aushape_format_is_valid(&coll->format) &&
           aushape_gbtree_is_valid(coll->gbtree) &&
           (coll->type->is_valid == NULL ||
            coll->type->is_valid(coll));
}

void
aushape_coll_destroy(struct aushape_coll *coll)
{
    assert(coll == NULL || aushape_coll_is_valid(coll));

    if (coll == NULL) {
        return;
    }
    if (coll->type->cleanup != NULL) {
        coll->type->cleanup(coll);
    }
    memset(coll, 0, coll->type->size);
    free(coll);
}

bool
aushape_coll_is_empty(const struct aushape_coll *coll)
{
    assert(aushape_coll_is_valid(coll));
    return coll->type->is_empty(coll);
}

void
aushape_coll_empty(struct aushape_coll *coll)
{
    assert(aushape_coll_is_valid(coll));
    coll->type->empty(coll);
    coll->ended = false;
}

bool
aushape_coll_is_ended(const struct aushape_coll *coll)
{
    assert(aushape_coll_is_valid(coll));
    return coll->ended;
}

enum aushape_rc
aushape_coll_add(struct aushape_coll *coll,
                 size_t *pcount,
                 size_t level,
                 size_t prio,
                 auparse_state_t *au)
{
    enum aushape_rc rc;
    if (!aushape_coll_is_valid(coll) || pcount == NULL || au == NULL) {
        return AUSHAPE_RC_INVALID_ARGS;
    }
    if (aushape_coll_is_ended(coll)) {
        return AUSHAPE_RC_INVALID_STATE;
    }
    rc = (coll->type->add != NULL)
                ? coll->type->add(coll, pcount, level, prio, au)
                : AUSHAPE_RC_OK;
    return rc;
}

enum aushape_rc
aushape_coll_end(struct aushape_coll *coll,
                 size_t *pcount,
                 size_t level,
                 size_t prio)
{
    enum aushape_rc rc;
    if (!aushape_coll_is_valid(coll) || pcount == NULL) {
        return AUSHAPE_RC_INVALID_ARGS;
    }
    if (aushape_coll_is_ended(coll)) {
        return AUSHAPE_RC_INVALID_STATE;
    }
    rc = (!aushape_coll_is_empty(coll) && coll->type->end != NULL)
                ? coll->type->end(coll, pcount, level, prio)
                : AUSHAPE_RC_OK;
    coll->ended = true;
    return rc;
}
