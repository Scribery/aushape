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
 * Evaluate an expression and return false if it is false.
 *
 * @param _expr The expression to evaluate.
 */
#define GUARD_BOOL(_expr) \
    do {                            \
        if (!(_expr)) {             \
            return false;           \
        }                           \
    } while (0)

/**
 * Evaluate an expression and return AUSHAPE_RC_NOMEM if it is false.
 *
 * @param _expr The expression to evaluate.
 */
#define GUARD_RC(_expr) \
    do {                                \
        if (!(_expr)) {                 \
            return AUSHAPE_RC_NOMEM;    \
        }                               \
    } while (0)

/**
 * Add a formatted fragment for an auparse field to a growing buffer.
 *
 * @param gbuf      The growing buffer to add the fragment to.
 * @param format    The output format to use.
 * @param level     Syntactic nesting level the field is output at.
 * @param first     True if this is the first field being output for a record,
 *                  false otherwise.
 * @param name      The field name.
 * @param au        The auparse state with the current field as the one to be
 *                  output.
 *
 * @return True if added succesfully, false if memory allocation failed.
 */
static bool
aushape_single_coll_add_field(struct aushape_gbuf *gbuf,
                              const struct aushape_format *format,
                              size_t level,
                              bool first,
                              const char *name,
                              auparse_state_t *au)
{
    size_t l = level;
    const char *value_r;
    const char *value_i = auparse_interpret_field(au);
    int type = auparse_get_field_type(au);

    assert(aushape_gbuf_is_valid(gbuf));
    assert(aushape_format_is_valid(format));
    assert(name != NULL);
    assert(au != NULL);

    switch (type) {
    case AUPARSE_TYPE_ESCAPED:
    case AUPARSE_TYPE_ESCAPED_KEY:
        value_r = NULL;
        break;
    default:
        value_r = auparse_get_field_str(au);
        if (strcmp(value_r, value_i) == 0) {
            value_r = NULL;
        }
        break;
    }

    switch (format->lang) {
    case AUSHAPE_LANG_XML:
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, l));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '<'));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, " i=\""));
        GUARD_BOOL(aushape_gbuf_add_str_xml(gbuf, value_i));
        if (value_r != NULL) {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\" r=\""));
            GUARD_BOOL(aushape_gbuf_add_str_xml(gbuf, value_r));
        }
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\"/>"));
        break;
    case AUSHAPE_LANG_JSON:
        if (!first) {
            GUARD_BOOL(aushape_gbuf_add_char(gbuf, ','));
        }
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, l));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\":["));
        l++;
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, l));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
        GUARD_BOOL(aushape_gbuf_add_str_json(gbuf, value_i));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
        if (value_r != NULL) {
            GUARD_BOOL(aushape_gbuf_add_char(gbuf, ','));
            GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, l));
            GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
            GUARD_BOOL(aushape_gbuf_add_str_json(gbuf, value_r));
            GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
        }
        l--;
        GUARD_BOOL(aushape_gbuf_space_closing(gbuf, format, l));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, ']'));
        break;
    default:
        assert(false);
        return false;
    }

    assert(l == level);
    return true;
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
 * @return True if added succesfully, false if memory allocation failed.
 */
static bool
aushape_single_coll_add_record(struct aushape_gbuf *gbuf,
                               const struct aushape_format *format,
                               size_t level,
                               bool first,
                               const char *name,
                               auparse_state_t *au)
{
    size_t l = level;
    bool first_field;
    const char *field_name;

    assert(aushape_gbuf_is_valid(gbuf));
    assert(aushape_format_is_valid(format));
    assert(au != NULL);

    if (format->lang == AUSHAPE_LANG_XML) {
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, l));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '<'));
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, " raw=\""));
        /* TODO: Return AUSHAPE_RC_AUPARSE_FAILED on failure */
        GUARD_BOOL(aushape_gbuf_add_str_xml(
                                        gbuf, auparse_get_record_text(au)));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\">"));
    } else {
        if (!first) {
            GUARD_BOOL(aushape_gbuf_add_char(gbuf, ','));
        }
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, l));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\":{"));
        l++;
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, l));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\"raw\":\""));
        /* TODO: Return AUSHAPE_RC_AUPARSE_FAILED on failure */
        GUARD_BOOL(aushape_gbuf_add_str_json(
                                        gbuf, auparse_get_record_text(au)));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\","));
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, l));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\"fields\":{"));
    }

    l++;
    auparse_first_field(au);
    first_field = true;
    do {
        field_name = auparse_get_field_name(au);
        if (strcmp(field_name, "type") != 0 &&
            strcmp(field_name, "node") != 0) {
            GUARD_BOOL(aushape_single_coll_add_field(gbuf, format, l,
                                                     first_field,
                                                     field_name, au));
            first_field = false;
        }
    } while (auparse_next_field(au) > 0);

    l--;
    if (format->lang == AUSHAPE_LANG_XML) {
        GUARD_BOOL(aushape_gbuf_space_closing(gbuf, format, l));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "</"));
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, ">"));
    } else {
        if (!first_field) {
            GUARD_BOOL(aushape_gbuf_space_closing(gbuf, format, l));
        }
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '}'));
        l--;
        GUARD_BOOL(aushape_gbuf_space_closing(gbuf, format, l));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '}'));
    }

    assert(l == level);
    return true;
}

static enum aushape_rc
aushape_single_coll_add(struct aushape_coll *coll,
                        size_t level,
                        bool *pfirst,
                        auparse_state_t *au)
{
    struct aushape_single_coll *single_coll =
                    (struct aushape_single_coll *)coll;
    const char *name;

    assert(aushape_coll_is_valid(coll));
    assert(pfirst != NULL);
    assert(au != NULL);

    name = auparse_get_type_name(au);
    if (name == NULL) {
        return AUSHAPE_RC_AUPARSE_FAILED;
    }
    if (aushape_single_coll_seen_has(coll, name)) {
        if (single_coll->unique) {
            return AUSHAPE_RC_REPEATED_RECORD;
        }
    } else {
        GUARD_BOOL(aushape_single_coll_seen_add(coll, name));
    }
    GUARD_BOOL(aushape_single_coll_add_record(coll->gbuf, &coll->format,
                                              level, *pfirst, name, au));
    *pfirst = false;
    return AUSHAPE_RC_OK;
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
