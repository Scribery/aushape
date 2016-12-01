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
    struct aushape_coll     coll;
    /** Name of the output container */
    const char             *name;
    /** Items growing buffer tree */
    struct aushape_gbtree   items;
};

static bool
aushape_rep_coll_is_valid(const struct aushape_coll *coll)
{
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    return rep_coll->name != NULL &&
           aushape_gbtree_is_valid(&rep_coll->items);
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
    aushape_gbtree_init(&rep_coll->items, 4096, 8, 8);

    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

static void
aushape_rep_coll_cleanup(struct aushape_coll *coll)
{
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    aushape_gbtree_cleanup(&rep_coll->items);
}

static bool
aushape_rep_coll_is_empty(const struct aushape_coll *coll)
{
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    return aushape_gbtree_is_empty(&rep_coll->items);
}

static void
aushape_rep_coll_empty(struct aushape_coll *coll)
{
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    aushape_gbtree_empty(&rep_coll->items);
}

static enum aushape_rc
aushape_rep_coll_add(struct aushape_coll *coll,
                     size_t *pcount,
                     size_t level,
                     size_t prio,
                     auparse_state_t *au)
{
    enum aushape_rc rc;
    size_t l = level;
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    struct aushape_gbtree *gbtree = &rep_coll->items;
    struct aushape_gbuf *gbuf = &gbtree->text;
    size_t len;

    (void)pcount;
    (void)prio;

    assert(aushape_coll_is_valid(coll));
    assert(pcount != NULL);
    assert(au != NULL);

    /* If it's the first record */
    if (aushape_gbtree_is_empty(gbtree)) {
        /* Output prologue */
        if (coll->format.lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_fmt(gbuf, "<%s>", rep_coll->name));
        } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_fmt(gbuf, "\"%s\":[", rep_coll->name));
        }
        /* Commit prologue item */
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(gbtree, 0));
    }

    /* Account for the container markup */
    l++;

    /*
     * Begin the item
     */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<item>"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        if (aushape_gbtree_get_node_num(gbtree) > 1) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
        }
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '{'));
    }

    l++;

    /*
     * Output the fields
     */
    len = gbuf->len;
    rc = aushape_record_format_fields(gbuf, &coll->format,
                                      l, au);
    if (rc != AUSHAPE_RC_OK) {
        assert(rc != AUSHAPE_RC_INVALID_ARGS);
        goto cleanup;
    }

    l--;

    /*
     * Finish the item
     */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</item>"));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        if (gbuf->len > len) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf,
                                                     &coll->format, l));
        }
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '}'));
    }

    /*
     * Commit the item
     */
    aushape_gbtree_node_add_text(gbtree,
                                 aushape_gbtree_get_node_num(gbtree) - 1);

    /* Account for the container markup */
    l--;

    assert(l == level);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

static enum aushape_rc
aushape_rep_coll_end(struct aushape_coll *coll,
                     size_t *pcount,
                     size_t level,
                     size_t prio)
{
    struct aushape_rep_coll *rep_coll =
                    (struct aushape_rep_coll *)coll;
    struct aushape_gbtree *gbtree = &rep_coll->items;
    struct aushape_gbuf *gbuf = &gbtree->text;
    enum aushape_rc rc = AUSHAPE_RC_OK;
    size_t l = level;

    assert(aushape_coll_is_valid(coll));
    assert(pcount != NULL);

    /* Output epilogue */
    if (coll->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &coll->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_fmt(gbuf, "</%s>", rep_coll->name));
    } else if (coll->format.lang == AUSHAPE_LANG_JSON) {
        if (!aushape_gbtree_is_empty(&rep_coll->items)) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &coll->format, l));
        }
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ']'));
    }
    /* Commit epilogue */
    AUSHAPE_GUARD(aushape_gbtree_node_add_text(gbtree, 0));

    /* Output record */
    if (coll->format.lang == AUSHAPE_LANG_JSON && *pcount > 0) {
        AUSHAPE_GUARD(aushape_gbuf_add_char(&coll->gbtree->text, ','));
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(coll->gbtree, prio));
    }
    AUSHAPE_GUARD(aushape_gbtree_node_add_tree(coll->gbtree, prio, gbtree));

    assert(l == level);
    (*pcount)++;
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
