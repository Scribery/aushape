/**
 * Generic repeated record collector
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

#include <aushape/rep_coll.h>
#include <aushape/coll.h>
#include <aushape/record.h>
#include <aushape/guard.h>
#include <string.h>

struct aushape_rep_coll {
    /** Abstract base collector */
    struct aushape_coll    coll;
    /** Name of the output container */
    const char *name;
    /** Raw log lines buffer */
    struct aushape_gbuf lines;
    /** Output log record buffer */
    struct aushape_gbuf items;
};

static bool
aushape_rep_coll_is_valid(const struct aushape_coll *coll)
{
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    return rep_coll->name != NULL &&
           aushape_gbuf_is_valid(&rep_coll->lines) &&
           aushape_gbuf_is_valid(&rep_coll->items);
}

static enum aushape_rc
aushape_rep_coll_init(struct aushape_coll *coll,
                      const void *args)
{
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    const struct aushape_rep_coll_args *rep_args =
                    (const struct aushape_rep_coll_args *)args;
    enum aushape_rc rc;

    AUSHAPE_GUARD_BOOL(INVALID_ARGS, rep_args != NULL &&
                                     rep_args->name != NULL);

    rep_coll->name = rep_args->name;
    aushape_gbuf_init(&rep_coll->lines);
    aushape_gbuf_init(&rep_coll->items);

    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

static void
aushape_rep_coll_cleanup(struct aushape_coll *coll)
{
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    aushape_gbuf_cleanup(&rep_coll->lines);
    aushape_gbuf_cleanup(&rep_coll->items);
}

static bool
aushape_rep_coll_is_empty(const struct aushape_coll *coll)
{
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    return aushape_gbuf_is_empty(&rep_coll->lines) &&
           aushape_gbuf_is_empty(&rep_coll->items);
}

static void
aushape_rep_coll_empty(struct aushape_coll *coll)
{
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    aushape_gbuf_empty(&rep_coll->lines);
    aushape_gbuf_empty(&rep_coll->items);
}

static enum aushape_rc
aushape_rep_coll_add(struct aushape_coll *coll,
                     size_t level,
                     bool *pfirst,
                     auparse_state_t *au)
{
    enum aushape_rc rc;
    size_t l = level;
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    struct aushape_gbuf *items;
    const char *raw;
    size_t len;

    (void)pfirst;

    assert(aushape_coll_is_valid(coll));
    assert(pfirst != NULL);
    assert(au != NULL);

    items = &rep_coll->items;

    /* Account for the container markup */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        l++;
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        l+=2;
    }

    /*
     * Add the raw line
     */
    raw = auparse_get_record_text(au);
    AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, raw != NULL);
    if (!aushape_gbuf_is_empty(&rep_coll->lines)) {
        AUSHAPE_GUARD(aushape_gbuf_add_char(&rep_coll->lines, '\n'));
    }
    AUSHAPE_GUARD(aushape_gbuf_add_str(&rep_coll->lines, raw));

    /*
     * Begin the output record
     */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(items, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(items, "<item>"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        if (!aushape_gbuf_is_empty(items)) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(items, ','));
        }
        AUSHAPE_GUARD(aushape_gbuf_space_opening(items, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(items, '{'));
    }

    l++;

    /*
     * Output the fields
     */
    len = items->len;
    rc = aushape_record_format_fields(items, &coll->format,
                                      l, au);
    if (rc != AUSHAPE_RC_OK) {
        assert(rc != AUSHAPE_RC_INVALID_ARGS);
        goto cleanup;
    }

    l--;

    /*
     * Finish the output record
     */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(items, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(items, "</item>"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        if (items->len > len) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(items,
                                                     &coll->format, l));
        }
        AUSHAPE_GUARD(aushape_gbuf_add_char(items, '}'));
    }

    /* Account for the container markup */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        l--;
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        l-=2;
    }

    assert(l == level);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

static enum aushape_rc
aushape_rep_coll_end(struct aushape_coll *coll,
                        size_t level,
                        bool *pfirst)
{
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    enum aushape_rc rc = AUSHAPE_RC_OK;
    struct aushape_gbuf *gbuf = coll->gbuf;
    size_t l = level;

    /* Output prologue */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_fmt(gbuf, "<%s raw=\"",
                                           rep_coll->name));
        AUSHAPE_GUARD(aushape_gbuf_add_buf_xml(gbuf,
                                               rep_coll->lines.ptr,
                                               rep_coll->lines.len));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\">"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        if (!*pfirst) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
        }
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_fmt(gbuf, "\"%s\":{", rep_coll->name));
        l++;
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"raw\":\""));
        AUSHAPE_GUARD(aushape_gbuf_add_buf_json(gbuf,
                                                rep_coll->lines.ptr,
                                                rep_coll->lines.len));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\","));
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '"'));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "items"));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\":["));
    }
    l++;

    /* Output items */
    AUSHAPE_GUARD(aushape_gbuf_add_buf(gbuf,
                                       rep_coll->items.ptr,
                                       rep_coll->items.len));

    l--;
    /* Output epilogue */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_fmt(gbuf, "</%s>", rep_coll->name));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        if (!aushape_gbuf_is_empty(&rep_coll->items)) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &coll->format, l));
        }
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ']'));
        l--;
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '}'));
    }

    assert(l == level);
    *pfirst = false;
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

const struct aushape_coll_type aushape_rep_coll_type = {
    .size       = sizeof(struct aushape_rep_coll),
    .init       = aushape_rep_coll_init,
    .is_valid   = aushape_rep_coll_is_valid,
    .cleanup    = aushape_rep_coll_cleanup,
    .is_empty   = aushape_rep_coll_is_empty,
    .empty      = aushape_rep_coll_empty,
    .add        = aushape_rep_coll_add,
    .end        = aushape_rep_coll_end,
};
