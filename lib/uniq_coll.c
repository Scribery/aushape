/**
 * Single (non-aggregated) record collector
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

#include <aushape/uniq_coll.h>
#include <aushape/coll.h>
#include <aushape/record.h>
#include <aushape/guard.h>
#include <string.h>

struct aushape_uniq_coll {
    /** Abstract base collector */
    struct aushape_coll    coll;
    /** Names of the record types seen, zero-terminated, one after another */
    struct aushape_gbuf seen;
};

static bool
aushape_uniq_coll_is_valid(const struct aushape_coll *coll)
{
    struct aushape_uniq_coll *uniq_coll =
                    (struct aushape_uniq_coll *)coll;
    return aushape_gbuf_is_valid(&uniq_coll->seen);
}

static enum aushape_rc
aushape_uniq_coll_init(struct aushape_coll *coll,
                       const void *args)
{
    struct aushape_uniq_coll *uniq_coll =
                    (struct aushape_uniq_coll *)coll;
    (void)args;
    aushape_gbuf_init(&uniq_coll->seen);
    return AUSHAPE_RC_OK;
}

static void
aushape_uniq_coll_cleanup(struct aushape_coll *coll)
{
    struct aushape_uniq_coll *uniq_coll =
                    (struct aushape_uniq_coll *)coll;
    aushape_gbuf_cleanup(&uniq_coll->seen);
}

static bool
aushape_uniq_coll_is_empty(const struct aushape_coll *coll)
{
    struct aushape_uniq_coll *uniq_coll =
                    (struct aushape_uniq_coll *)coll;
    return aushape_gbuf_is_empty(&uniq_coll->seen);
}

static void
aushape_uniq_coll_empty(struct aushape_coll *coll)
{
    struct aushape_uniq_coll *uniq_coll =
                    (struct aushape_uniq_coll *)coll;
    aushape_gbuf_empty(&uniq_coll->seen);
}

/**
 * Check if a record type is present in the list of record types seen by a
 * unique collector.
 *
 * @param coll  The collector to check the seen record type list of.
 * @param name  The name of the record type to check.
 *
 * @return True if the record type was seen, false otherwise.
 */
static bool
aushape_uniq_coll_seen_has(struct aushape_coll *coll,
                           const char *name)
{
    struct aushape_uniq_coll *uniq_coll =
                    (struct aushape_uniq_coll *)coll;
    struct aushape_gbuf *gbuf;
    const char *p;

    assert(aushape_coll_is_valid(coll));
    assert(coll->type == &aushape_uniq_coll_type);
    assert(name != NULL);

    gbuf = &uniq_coll->seen;
    p = gbuf->ptr;

    while ((size_t)(p - gbuf->ptr) < gbuf->len) {
        if (strcmp(name, p) == 0) {
            return true;
        }
        p += strlen(p) + 1;
    }

    return false;
}

/**
 * Add a record type to the list of record types seen by a unique collector.
 *
 * @param coll  The collector to add the type to.
 * @param name  The name of the record type to add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
static enum aushape_rc
aushape_uniq_coll_seen_add(struct aushape_coll *coll,
                           const char *name)
{
    struct aushape_uniq_coll *uniq_coll =
                    (struct aushape_uniq_coll *)coll;
    assert(aushape_coll_is_valid(coll));
    assert(coll->type == &aushape_uniq_coll_type);
    assert(name != NULL);
    return aushape_gbuf_add_buf(&uniq_coll->seen, name, strlen(name) + 1);
}

static enum aushape_rc
aushape_uniq_coll_add(struct aushape_coll *coll,
                      size_t level,
                      bool *pfirst,
                      auparse_state_t *au)
{
    enum aushape_rc rc;
    const char *name;

    assert(aushape_coll_is_valid(coll));
    assert(pfirst != NULL);
    assert(au != NULL);

    name = auparse_get_type_name(au);
    AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, name != NULL);
    if (aushape_uniq_coll_seen_has(coll, name)) {
        rc = AUSHAPE_RC_REPEATED_RECORD;
        goto cleanup;
    } else {
        AUSHAPE_GUARD(aushape_uniq_coll_seen_add(coll, name));
    }
    rc = aushape_record_format(coll->gbuf, &coll->format,
                               level, *pfirst, name, au);
    if (rc != AUSHAPE_RC_OK) {
        assert(rc != AUSHAPE_RC_INVALID_ARGS);
        goto cleanup;
    }
    *pfirst = false;
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

const struct aushape_coll_type aushape_uniq_coll_type = {
    .size       = sizeof(struct aushape_uniq_coll),
    .init       = aushape_uniq_coll_init,
    .is_valid   = aushape_uniq_coll_is_valid,
    .cleanup    = aushape_uniq_coll_cleanup,
    .is_empty   = aushape_uniq_coll_is_empty,
    .empty      = aushape_uniq_coll_empty,
    .add        = aushape_uniq_coll_add,
};
