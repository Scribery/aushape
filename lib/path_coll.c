/**
 * Path record collector
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

#include <aushape/path_coll.h>
#include <aushape/coll.h>
#include <aushape/field.h>
#include <aushape/guard.h>
#include <string.h>
#include <stdio.h>

/** Location of an item in the output buffer */
struct aushape_path_coll_loc {
    /** Offset */
    size_t  off;
    /** Length */
    size_t  len;
};

/** Minimum number of allocated item locations */
#define AUSHAPE_PATH_COLL_LOC_SIZE_MIN  16

/** Maximum number of allocated item locations */
#define AUSHAPE_PATH_COLL_LOC_SIZE_MAX  4096

/** Path record collector */
struct aushape_path_coll {
    /** Abstract base collector */
    struct aushape_coll             coll;
    /** Raw log buffer */
    struct aushape_gbuf             raw;
    /** Buffer for items, in order of addition, without separation */
    struct aushape_gbuf             items;
    /**
     * Item locations within the buffer, in the proper order.
     * Zero item length means item wasn't encountered yet.
     */
    struct aushape_path_coll_loc   *loc_list;
    /** Number of allocated item locations */
    size_t                          loc_size;
    /** Number of valid item locations (max index seen + 1) */
    size_t                          loc_num;
};

static bool
aushape_path_coll_is_valid(const struct aushape_coll *coll)
{
    struct aushape_path_coll *path_coll = (struct aushape_path_coll *)coll;
    return aushape_gbuf_is_valid(&path_coll->raw) &&
           aushape_gbuf_is_valid(&path_coll->items) &&
           path_coll->loc_list != NULL &&
           path_coll->loc_size >= AUSHAPE_PATH_COLL_LOC_SIZE_MIN &&
           path_coll->loc_size <= AUSHAPE_PATH_COLL_LOC_SIZE_MAX &&
           path_coll->loc_num <= path_coll->loc_size;
}

static enum aushape_rc
aushape_path_coll_init(struct aushape_coll *coll, const void *args)
{
    struct aushape_path_coll *path_coll = (struct aushape_path_coll *)coll;
    (void)args;
    aushape_gbuf_init(&path_coll->raw);
    aushape_gbuf_init(&path_coll->items);
    path_coll->loc_size = AUSHAPE_PATH_COLL_LOC_SIZE_MIN;
    path_coll->loc_list = malloc(sizeof(*path_coll->loc_list) *
                                 path_coll->loc_size);
    return AUSHAPE_RC_OK;
}

static void
aushape_path_coll_cleanup(struct aushape_coll *coll)
{
    struct aushape_path_coll *path_coll = (struct aushape_path_coll *)coll;
    free(path_coll->loc_list);
    aushape_gbuf_cleanup(&path_coll->items);
    aushape_gbuf_cleanup(&path_coll->raw);
}

static bool
aushape_path_coll_is_empty(const struct aushape_coll *coll)
{
    struct aushape_path_coll *path_coll =
                    (struct aushape_path_coll *)coll;
    return path_coll->loc_num == 0;
}

static void
aushape_path_coll_empty(struct aushape_coll *coll)
{
    struct aushape_path_coll *path_coll = (struct aushape_path_coll *)coll;
    aushape_gbuf_empty(&path_coll->raw);
    aushape_gbuf_empty(&path_coll->items);
    path_coll->loc_num = 0;
}

static enum aushape_rc
aushape_path_coll_add(struct aushape_coll *coll,
                      size_t level,
                      bool *pfirst,
                      auparse_state_t *au)
{
    struct aushape_path_coll *path_coll = (struct aushape_path_coll *)coll;
    enum aushape_rc rc;
    size_t l = level;
    const char *raw;
    size_t start;
    bool first_field = true;
    const char *field_name;
    const char *field_value;
    bool got_idx = false;
    size_t idx;
    int end;

    (void)pfirst;

    /* Remember the new item start offset in the buffer */
    start = path_coll->items.len;

    /* Account for the record markup */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        l++;
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        l+=2;
    }

    /*
     * Add raw record
     */
    if (!aushape_gbuf_is_empty(&path_coll->raw)) {
        AUSHAPE_GUARD(aushape_gbuf_add_char(&path_coll->raw, '\n'));
    }
    raw = auparse_get_record_text(au);
    AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, raw != NULL);
    AUSHAPE_GUARD(aushape_gbuf_add_str(&path_coll->raw, raw));

    /*
     * Begin the item
     */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(&path_coll->items,
                                                 &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(&path_coll->items, "<item>"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(&path_coll->items,
                                                 &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(&path_coll->items, '{'));
    }
    l++;

    /*
     * For each field in the record
     */
    AUSHAPE_GUARD_BOOL(INVALID_PATH, auparse_first_field(au) != 0);
    do {
        field_name = auparse_get_field_name(au);
        AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, field_name != NULL);
        /* If it's the "type" pseudo-field */
        if (strcmp(field_name, "type") == 0) {
            continue;
        /* If it's the "node" which we handle at event level */
        } else if (strcmp(field_name, "node") == 0) {
            continue;
        /* If it's the item index */
        } else if (strcmp(field_name, "item") == 0) {
            /* Make sure we haven't seen it yet */
            AUSHAPE_GUARD_BOOL(INVALID_PATH, !got_idx);

            /* Parse the value */
            field_value = auparse_get_field_str(au);
            AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, field_value != NULL);
            end = 0;
            AUSHAPE_GUARD_BOOL(
                    INVALID_PATH,
                    sscanf(field_value, "%zu%n", &idx, &end) >= 1 &&
                    (size_t)end == strlen(field_value));

            /* Make sure the space for the item is allocated */
            if (idx > path_coll->loc_size) {
                struct aushape_path_coll_loc *new_loc_list;
                size_t new_loc_size = path_coll->loc_size;
                do {
                    new_loc_size *= 2;
                    AUSHAPE_GUARD_BOOL(
                            INVALID_PATH,
                            new_loc_size <= AUSHAPE_PATH_COLL_LOC_SIZE_MAX);
                } while (idx >= new_loc_size);
                new_loc_list = realloc(path_coll->loc_list,
                                       sizeof(*new_loc_list) * new_loc_size);
                AUSHAPE_GUARD_BOOL(NOMEM, new_loc_list != NULL);
                path_coll->loc_list = new_loc_list;
                memset(path_coll->loc_list + path_coll->loc_size, 0,
                       (new_loc_size - path_coll->loc_size) *
                        sizeof(*new_loc_list));
                path_coll->loc_size = new_loc_size;
            }

            /* Make sure the item slot is accounted for and is unnocupied */
            if (idx >= path_coll->loc_num) {
                path_coll->loc_num = idx + 1;
            } else {
                AUSHAPE_GUARD_BOOL(INVALID_PATH,
                                   path_coll->loc_list[idx].len == 0);
            }

            /* Record the item start index */
            path_coll->loc_list[idx].off = start;
            got_idx = true;
        /* If it's something else */
        } else {
            /* Add the field */
            AUSHAPE_GUARD(aushape_field_format(&path_coll->items,
                                               &coll->format, l,
                                               first_field,
                                               field_name, au));
            first_field = false;
        }
    } while (auparse_next_field(au) > 0);

    /* Make sure we found the index */
    AUSHAPE_GUARD_BOOL(INVALID_PATH, got_idx);

    /*
     * Finish the item
     */
    l--;
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(&path_coll->items,
                                                 &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(&path_coll->items, "</item>"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(&path_coll->items,
                                                 &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(&path_coll->items, '}'));
    }

    /* Account for the record markup */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        l--;
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        l-=2;
    }

    assert(l == level);

    /* Record item length */
    path_coll->loc_list[idx].len = path_coll->items.len - start;

    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

static enum aushape_rc
aushape_path_coll_end(struct aushape_coll *coll,
                      size_t level,
                      bool *pfirst)
{
    struct aushape_path_coll *path_coll =
                    (struct aushape_path_coll *)coll;
    enum aushape_rc rc = AUSHAPE_RC_OK;
    struct aushape_gbuf *gbuf = coll->gbuf;
    size_t l = level;
    size_t idx;
    const char *ptr;
    size_t len;

    /* Output prologue */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<path raw=\""));
        AUSHAPE_GUARD(aushape_gbuf_add_buf_xml(gbuf,
                                               path_coll->raw.ptr,
                                               path_coll->raw.len));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\">"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        if (!*pfirst) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
        }
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"path\":{"));
        l++;
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"raw\":\""));
        AUSHAPE_GUARD(aushape_gbuf_add_buf_json(gbuf,
                                                path_coll->raw.ptr,
                                                path_coll->raw.len));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\","));
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"items\":["));
    }
    l++;

    /* Output items */
    for (idx = 0; idx < path_coll->loc_num; idx++) {
        /* Add separator, if necessary */
        if (coll->format.lang == AUSHAPE_LANG_JSON) {
            /* If it's not the first item */
            if (idx > 0) {
                AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
            }
        }
        /* Get the item location and length */
        ptr = path_coll->items.ptr + path_coll->loc_list[idx].off;
        len = path_coll->loc_list[idx].len;
        /* Check that the item is not missing */
        AUSHAPE_GUARD_BOOL(INVALID_PATH, len != 0);
        /* Add the item */
        AUSHAPE_GUARD(aushape_gbuf_add_buf(gbuf, ptr, len));
    }

    l--;
    /* Output epilogue */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</path>"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        if (path_coll->loc_num > 0) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf,
                                                     &coll->format, l));
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

const struct aushape_coll_type aushape_path_coll_type = {
    .size       = sizeof(struct aushape_path_coll),
    .init       = aushape_path_coll_init,
    .is_valid   = aushape_path_coll_is_valid,
    .cleanup    = aushape_path_coll_cleanup,
    .is_empty   = aushape_path_coll_is_empty,
    .empty      = aushape_path_coll_empty,
    .add        = aushape_path_coll_add,
    .end        = aushape_path_coll_end,
};
