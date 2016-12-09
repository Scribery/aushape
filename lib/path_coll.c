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

/** Maximum accepted path item index */
#define AUSHAPE_PATH_COLL_MAX_IDX   255

/** Path record collector */
struct aushape_path_coll {
    /** Abstract base collector */
    struct aushape_coll             coll;
    /** Growing buffer tree for output */
    struct aushape_gbtree           gbtree;
};

static bool
aushape_path_coll_is_valid(const struct aushape_coll *coll)
{
    struct aushape_path_coll *path_coll = (struct aushape_path_coll *)coll;
    return aushape_gbtree_is_valid(&path_coll->gbtree);
}

static enum aushape_rc
aushape_path_coll_init(struct aushape_coll *coll, const void *args)
{
    struct aushape_path_coll *path_coll = (struct aushape_path_coll *)coll;
    (void)args;
    aushape_gbtree_init(&path_coll->gbtree, 2048, 8, 8);
    return AUSHAPE_RC_OK;
}

static void
aushape_path_coll_cleanup(struct aushape_coll *coll)
{
    struct aushape_path_coll *path_coll = (struct aushape_path_coll *)coll;
    aushape_gbtree_cleanup(&path_coll->gbtree);
}

static bool
aushape_path_coll_is_empty(const struct aushape_coll *coll)
{
    struct aushape_path_coll *path_coll =
                    (struct aushape_path_coll *)coll;
    return aushape_gbtree_is_empty(&path_coll->gbtree);
}

static void
aushape_path_coll_empty(struct aushape_coll *coll)
{
    struct aushape_path_coll *path_coll = (struct aushape_path_coll *)coll;
    aushape_gbtree_empty(&path_coll->gbtree);
}

static enum aushape_rc
aushape_path_coll_add(struct aushape_coll *coll,
                      size_t *pcount,
                      size_t level,
                      size_t prio,
                      auparse_state_t *au)
{
    struct aushape_path_coll *path_coll = (struct aushape_path_coll *)coll;
    struct aushape_gbtree *gbtree = &path_coll->gbtree;
    struct aushape_gbuf *gbuf = &gbtree->text;
    enum aushape_rc rc;
    size_t l = level;
    bool first_field = true;
    const char *field_name;
    const char *field_value;
    bool got_idx = false;
    size_t idx;
    int end;
    size_t node_idx;

    (void)pcount;
    (void)prio;

    /* If haven't output anything yet */
    if (aushape_gbtree_is_empty(gbtree)) {
        /* Output prologue */
        if (coll->format.lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<path>"));
        } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"path\":["));
        }
        /* Commit prologue item */
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(gbtree, 0));
    }

    /* Account for the record markup */
    l++;

    /*
     * Begin the item
     */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<item>"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '{'));
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
                    (size_t)end == strlen(field_value) &&
                    idx <= AUSHAPE_PATH_COLL_MAX_IDX);

            got_idx = true;
        /* If it's something else */
        } else {
            /* Add the field */
            AUSHAPE_GUARD(aushape_field_format(gbuf,
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
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</item>"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '}'));
    }

    /*
     * Commit the item, leaving space for separator items, if needed
     */
    node_idx = ((coll->format.lang == AUSHAPE_LANG_JSON) ? idx * 2 : idx) + 1;
    AUSHAPE_GUARD_BOOL(INVALID_PATH,
                       !aushape_gbtree_node_exists(gbtree, node_idx));
    AUSHAPE_GUARD(aushape_gbtree_node_put_text(gbtree, node_idx, idx));

    /* Account for the record markup */
    l--;

    assert(l == level);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

static enum aushape_rc
aushape_path_coll_end(struct aushape_coll *coll,
                      size_t *pcount,
                      size_t level,
                      size_t prio)
{
    struct aushape_path_coll *path_coll =
                    (struct aushape_path_coll *)coll;
    struct aushape_gbtree *gbtree = &path_coll->gbtree;
    struct aushape_gbuf *gbuf = &gbtree->text;
    enum aushape_rc rc = AUSHAPE_RC_OK;
    size_t idx;

    assert(aushape_coll_is_valid(coll));
    assert(pcount != NULL);

    /* Insert item separators, if needed */
    if (aushape_gbtree_get_node_num(gbtree) > 1) {
        if (coll->format.lang == AUSHAPE_LANG_JSON) {
            for (idx = 1;
                 idx <= (aushape_gbtree_get_node_num(gbtree) - 1) / 2;
                 idx++) {
                /* TODO Reuse the comma, don't copy */
                AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
                AUSHAPE_GUARD(aushape_gbtree_node_put_text(gbtree,
                                                           idx * 2, idx));
            }
        }
    }

    /* Check that all items were received */
    AUSHAPE_GUARD_BOOL(INVALID_PATH,
                       aushape_gbtree_is_solid(gbtree));

    /* Output epilogue */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &coll->format, level));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</path>"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        if (aushape_gbtree_get_node_num(gbtree) > 1) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf,
                                                     &coll->format, level));
        }
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ']'));
    }
    /* Commit epilogue item */
    AUSHAPE_GUARD(aushape_gbtree_node_add_text(gbtree, 0));

    /* Commit record */
    if (coll->format.lang == AUSHAPE_LANG_JSON && *pcount > 0) {
        AUSHAPE_GUARD(aushape_gbuf_add_char(&coll->gbtree->text, ','));
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(coll->gbtree, prio));
    }
    AUSHAPE_GUARD(aushape_gbtree_node_add_tree(coll->gbtree, prio, gbtree));

    (*pcount)++;
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
