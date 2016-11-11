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

#include <aushape/single_coll.h>
#include <aushape/coll.h>
#include <aushape/field.h>
#include <aushape/guard.h>
#include <string.h>

static const struct aushape_single_coll_args
                        aushape_single_coll_args_default = {
    .unique = true
};

struct aushape_single_coll {
    /** Abstract base collector */
    struct aushape_coll    coll;
    /** Do not allow adding duplicate record types, if true */
    bool    unique;
    /** Names of the record types seen, zero-terminated, one after another */
    struct aushape_gbuf seen;
};

static bool
aushape_single_coll_is_valid(const struct aushape_coll *coll)
{
    struct aushape_single_coll *single_coll =
                    (struct aushape_single_coll *)coll;
    return aushape_gbuf_is_valid(&single_coll->seen);
}

static enum aushape_rc
aushape_single_coll_init(struct aushape_coll *coll,
                         const void *args)
{
    struct aushape_single_coll *single_coll =
                    (struct aushape_single_coll *)coll;
    const struct aushape_single_coll_args *single_args =
                    (const struct aushape_single_coll_args *)args;
    if (single_args == NULL) {
        single_args = &aushape_single_coll_args_default;
    }
    single_coll->unique = single_args->unique;
    aushape_gbuf_init(&single_coll->seen);
    return AUSHAPE_RC_OK;
}

static void
aushape_single_coll_cleanup(struct aushape_coll *coll)
{
    struct aushape_single_coll *single_coll =
                    (struct aushape_single_coll *)coll;
    aushape_gbuf_cleanup(&single_coll->seen);
}

static bool
aushape_single_coll_is_empty(const struct aushape_coll *coll)
{
    struct aushape_single_coll *single_coll =
                    (struct aushape_single_coll *)coll;
    return aushape_gbuf_is_empty(&single_coll->seen);
}

static void
aushape_single_coll_empty(struct aushape_coll *coll)
{
    struct aushape_single_coll *single_coll =
                    (struct aushape_single_coll *)coll;
    aushape_gbuf_empty(&single_coll->seen);
}

/**
 * Check if a record type is present in the list of record types seen by a
 * single collector.
 *
 * @param coll  The collector to check the seen record type list of.
 * @param name  The name of the record type to check.
 *
 * @return True if the record type was seen, false otherwise.
 */
static bool
aushape_single_coll_seen_has(struct aushape_coll *coll,
                             const char *name)
{
    struct aushape_single_coll *single_coll =
                    (struct aushape_single_coll *)coll;
    struct aushape_gbuf *gbuf;
    const char *p;

    assert(aushape_coll_is_valid(coll));
    assert(coll->type == &aushape_single_coll_type);
    assert(name != NULL);

    gbuf = &single_coll->seen;
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
 * Add a record type to the list of record types seen by a single collector.
 *
 * @param coll  The collector to add the type to.
 * @param name  The name of the record type to add.
 *
 * @return True if added succesfully, false if memory allocation failed.
 */
static bool
aushape_single_coll_seen_add(struct aushape_coll *coll,
                             const char *name)
{
    struct aushape_single_coll *single_coll =
                    (struct aushape_single_coll *)coll;
    assert(aushape_coll_is_valid(coll));
    assert(coll->type == &aushape_single_coll_type);
    assert(name != NULL);
    return aushape_gbuf_add_buf(&single_coll->seen, name, strlen(name) + 1);
}

/**
 * Add a formatted fragment for an auparse record to a growing buffer.
 *
 * @param gbuf      The growing buffer to add the fragment to.
 * @param format    The output format to use.
 * @param level     Syntactic nesting level the record is output at.
 * @param first     True if this is the first record being output,
 *                  false otherwise.
 * @param name      The record (type) name.
 * @param au        The auparse state with the current record as the one to be
 *                  output.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK               - added succesfully,
 *          AUSHAPE_RC_INVALID_ARGS     - invalid arguments supplied,
 *          AUSHAPE_RC_NOMEM            - memory allocation failed,
 *          AUSHAPE_RC_AUPARSE_FAILED   - an auparse call failed.
 */
static enum aushape_rc
aushape_single_coll_add_record(struct aushape_gbuf *gbuf,
                               const struct aushape_format *format,
                               size_t level,
                               bool first,
                               const char *name,
                               auparse_state_t *au)
{
    enum aushape_rc rc;
    size_t l = level;
    bool first_field;
    const char *field_name;
    const char *raw;

    if (!aushape_gbuf_is_valid(gbuf) ||
        !aushape_format_is_valid(format) ||
        au == NULL) {
        rc = AUSHAPE_RC_INVALID_ARGS;
        goto cleanup;
    }

    if (format->lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_char(gbuf, '<'));
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_str_lowercase(gbuf, name));
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_str(gbuf, " raw=\""));
        raw = auparse_get_record_text(au);
        AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, raw != NULL);
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_str_xml(gbuf, raw));
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_str(gbuf, "\">"));
    } else {
        if (!first) {
            AUSHAPE_GUARD_BOOL(NOMEM,
                               aushape_gbuf_add_char(gbuf, ','));
        }
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_char(gbuf, '"'));
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_str_lowercase(gbuf, name));
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_str(gbuf, "\":{"));
        l++;
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_str(gbuf, "\"raw\":\""));
        raw = auparse_get_record_text(au);
        AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, raw != NULL);
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_str_json(gbuf, raw));
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_str(gbuf, "\","));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD_BOOL(NOMEM, aushape_gbuf_add_str(gbuf, "\"fields\":{"));
    }

    l++;
    auparse_first_field(au);
    first_field = true;
    do {
        field_name = auparse_get_field_name(au);
        if (strcmp(field_name, "type") != 0 &&
            strcmp(field_name, "node") != 0) {
            rc = aushape_field_format(gbuf, format, l,
                                      first_field,
                                      field_name, au);
            if (rc != AUSHAPE_RC_OK) {
                assert(rc != AUSHAPE_RC_INVALID_ARGS);
                goto cleanup;
            }
            first_field = false;
        }
    } while (auparse_next_field(au) > 0);

    l--;
    if (format->lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_space_closing(gbuf, format, l));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_str(gbuf, "</"));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_str_lowercase(gbuf, name));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_str(gbuf, ">"));
    } else {
        if (!first_field) {
            AUSHAPE_GUARD_BOOL(NOMEM,
                               aushape_gbuf_space_closing(gbuf, format, l));
        }
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_char(gbuf, '}'));
        l--;
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_space_closing(gbuf, format, l));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_char(gbuf, '}'));
    }

    assert(l == level);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

static enum aushape_rc
aushape_single_coll_add(struct aushape_coll *coll,
                        size_t level,
                        bool *pfirst,
                        auparse_state_t *au)
{
    enum aushape_rc rc;
    struct aushape_single_coll *single_coll =
                    (struct aushape_single_coll *)coll;
    const char *name;

    assert(aushape_coll_is_valid(coll));
    assert(pfirst != NULL);
    assert(au != NULL);

    name = auparse_get_type_name(au);
    AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, name != NULL);
    if (aushape_single_coll_seen_has(coll, name)) {
        if (single_coll->unique) {
            rc = AUSHAPE_RC_REPEATED_RECORD;
            goto cleanup;
        }
    } else {
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_single_coll_seen_add(coll, name));
    }
    rc = aushape_single_coll_add_record(coll->gbuf, &coll->format,
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

const struct aushape_coll_type aushape_single_coll_type = {
    .size       = sizeof(struct aushape_single_coll),
    .init       = aushape_single_coll_init,
    .is_valid   = aushape_single_coll_is_valid,
    .cleanup    = aushape_single_coll_cleanup,
    .is_empty   = aushape_single_coll_is_empty,
    .empty      = aushape_single_coll_empty,
    .add        = aushape_single_coll_add,
};
