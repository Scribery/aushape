/**
 * An (exponentially) growing buffer tree with trimming support.
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

#include <aushape/gbtree.h>
#include <aushape/gbnode.h>
#include <aushape/guard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

bool
aushape_gbtree_is_valid(const struct aushape_gbtree *gbtree)
{
    return gbtree != NULL &&
           aushape_gbuf_is_valid(&gbtree->text) &&
           aushape_garr_is_valid(&gbtree->nodes) &&
           aushape_garr_is_valid(&gbtree->prios) &&
           gbtree->tail <= gbtree->text.len;
}

void
aushape_gbtree_init(struct aushape_gbtree *gbtree,
                    size_t text_min, size_t node_min, size_t prio_min)
{
    assert(gbtree != NULL);
    assert(text_min != 0);
    assert(node_min != 0);
    assert(prio_min != 0);

    memset(gbtree, 0, sizeof(*gbtree));
    aushape_gbuf_init(&gbtree->text, text_min);
    aushape_garr_init(&gbtree->nodes,
                      sizeof(struct aushape_gbnode), node_min);
    aushape_garr_init(&gbtree->prios, sizeof(size_t), prio_min);
    assert(aushape_gbtree_is_valid(gbtree));
}

void
aushape_gbtree_cleanup(struct aushape_gbtree *gbtree)
{
    assert(aushape_gbtree_is_valid(gbtree));
    aushape_gbuf_cleanup(&gbtree->text);
    aushape_garr_cleanup(&gbtree->nodes);
    aushape_garr_cleanup(&gbtree->prios);
    memset(gbtree, 0, sizeof(*gbtree));
}

void
aushape_gbtree_empty(struct aushape_gbtree *gbtree)
{
    assert(aushape_gbtree_is_valid(gbtree));
    aushape_gbuf_empty(&gbtree->text);
    aushape_garr_empty(&gbtree->nodes);
    aushape_garr_empty(&gbtree->prios);
    gbtree->tail = 0;
}

bool
aushape_gbtree_is_empty(const struct aushape_gbtree *gbtree)
{
    size_t i;

    assert(aushape_gbtree_is_valid(gbtree));

    for (i = 0; i < aushape_garr_get_len(&gbtree->nodes); i++) {
        if (!aushape_gbnode_is_empty(
                    aushape_garr_const_get(&gbtree->nodes, i))) {
            return false;
        }
    }

    return true;
}

bool
aushape_gbtree_is_solid(const struct aushape_gbtree *gbtree)
{
    size_t i;

    assert(aushape_gbtree_is_valid(gbtree));

    for (i = 0; i < aushape_garr_get_len(&gbtree->nodes); i++) {
        if (!aushape_gbnode_is_solid(
                    aushape_garr_const_get(&gbtree->nodes, i))) {
            return false;
        }
    }

    return true;
}

bool
aushape_gbtree_is_atomic(struct aushape_gbtree *gbtree, bool cached)
{
    size_t prio;
    struct aushape_garr *prios = &gbtree->prios;

    assert(aushape_gbtree_is_valid(gbtree));

    if (cached) {
        return gbtree->atomic;
    }

    /* We are not atomic, if we have non-atomic nodes with priority zero */
    if (aushape_garr_get_len(prios) > 0) {
        size_t head_index = *(size_t *)aushape_garr_get(prios, 0);
        /* If we have nodes with priority zero */
        if (~head_index != 0) {
            struct aushape_gbnode *node;
            size_t index = head_index;
            do {
                node = aushape_garr_get(&gbtree->nodes, index);
                if (!aushape_gbnode_is_atomic(node, false)) {
                    return (gbtree->atomic = false);
                }
                index = node->next_index;
            } while (index != head_index);
        }
    }

    /* We are not atomic, if we have nodes with priority > 0 */
    for (prio = 1; prio < aushape_garr_get_len(prios); prio++) {
        if (~*(size_t *)aushape_garr_get(prios, prio) != 0) {
            return (gbtree->atomic = false);
        }
    }

    /* Otherwise we're atomic */
    return (gbtree->atomic = true);
}

size_t
aushape_gbtree_get_len(struct aushape_gbtree *gbtree, bool cached)
{
    assert(aushape_gbtree_is_valid(gbtree));

    if (!cached) {
        size_t len = 0;
        size_t i;

        for (i = 0; i < aushape_garr_get_len(&gbtree->nodes); i++) {
            struct aushape_gbnode *node;
            node = aushape_garr_get(&gbtree->nodes, i);
            len += aushape_gbnode_get_len(node, cached);
        }

        gbtree->len = len;
    }

    return gbtree->len;
}

bool
aushape_gbtree_node_exists(const struct aushape_gbtree *gbtree, size_t index)
{
    const struct aushape_garr *nodes = &gbtree->nodes;

    assert(aushape_gbtree_is_valid(gbtree));

    if (index >= aushape_garr_get_len(nodes)) {
        return false;
    } else {
        const struct aushape_gbnode *node =
                            aushape_garr_const_get(nodes, index);
        return node->type != AUSHAPE_GBNODE_TYPE_VOID;
    }
}

enum aushape_rc
aushape_gbtree_node_void(struct aushape_gbtree *gbtree, size_t index)
{
    enum aushape_rc rc;
    struct aushape_garr *nodes = &gbtree->nodes;
    size_t nodes_len;
    struct aushape_garr *prios = &gbtree->prios;
    struct aushape_gbnode *node;

    assert(aushape_gbtree_is_valid(gbtree));

    /**
     * Reset or allocate the target node
     */
    nodes_len = aushape_garr_get_len(nodes);
    /* If node is allocated already */
    if (index < nodes_len) {
        /* Get its pointer */
        node = aushape_garr_get(nodes, index);
        /* If it's valid */
        if (node->type != AUSHAPE_GBNODE_TYPE_VOID) {
            /* If it's the only node in its peer list */
            if (node->next_index == index) {
                /* Remove the list */
                aushape_garr_set_byte_span(prios, node->prio, 0xff, 1);
            } else {
                struct aushape_gbnode *prev_node;
                struct aushape_gbnode *next_node;

                /* Unlink it from its priority peer list */
                prev_node = aushape_garr_get(nodes, node->prev_index);
                next_node = aushape_garr_get(nodes, node->next_index);
                prev_node->next_index = node->next_index;
                next_node->prev_index = node->prev_index;

                /* If it's the head node of the peer list */
                if (*(size_t *)aushape_garr_get(prios, node->prio) == index) {
                    /* Make the next node the new head */
                    aushape_garr_set(prios, node->prio, &node->next_index);
                }
            }
            node->type = AUSHAPE_GBNODE_TYPE_VOID;
        }
    } else {
        /* Allocate the node */
        AUSHAPE_GUARD(aushape_garr_add_zero_span(nodes,
                                                 index - nodes_len + 1));
    }

    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

static enum aushape_rc
aushape_gbtree_node_put(struct aushape_gbtree *gbtree,
                        size_t index, size_t prio,
                        struct aushape_gbnode **pnode)
{
    enum aushape_rc rc;
    struct aushape_garr *nodes = &gbtree->nodes;
    struct aushape_garr *prios = &gbtree->prios;
    size_t prios_len;
    struct aushape_gbnode *node;

    assert(aushape_gbtree_is_valid(gbtree));
    assert(pnode != NULL);

    /**
     * Reset or allocate the target node
     */
    AUSHAPE_GUARD(aushape_gbtree_node_void(gbtree, index));

    /* Here the node should be considered not valid, uninitialized */
    node = aushape_garr_get(nodes, index);
    node->owner = gbtree;
    node->prio = prio;

    /**
     * Add the node to the priority level node list
     */
    prios_len = aushape_garr_get_len(prios);
    /* If node priority is allocated */
    if (prio < prios_len) {
        size_t head_index = *(size_t *)aushape_garr_get(prios, prio);
        /* If priority is uninitialized */
        if (~head_index == 0) {
            /* Make single-entry priority level list */
            node->prev_index = index;
            node->next_index = index;
        } else {
            struct aushape_gbnode *head_node;
            size_t tail_index;
            struct aushape_gbnode *tail_node;
            /* Add node to the start of the list */
            head_node = aushape_garr_get(nodes, head_index);
            tail_index = head_node->prev_index;
            tail_node = aushape_garr_get(nodes, tail_index);

            tail_node->next_index = index;
            node->prev_index = tail_index;
            head_node->prev_index = index;
            node->next_index = head_index;
        }
    } else {
        /* Allocate priority level */
        AUSHAPE_GUARD(aushape_garr_add_byte_span(prios, 0xff,
                                                 prio - prios_len + 1));
        /* Make single-entry priority level list */
        node->prev_index = index;
        node->next_index = index;
    }

    /* Here the priority entry should be considered not valid, uninitialized */

    /* Store the new priority level list head index */
    aushape_garr_set(prios, prio, &index);

    *pnode = node;

    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

enum aushape_rc
aushape_gbtree_node_put_text(struct aushape_gbtree *gbtree,
                             size_t index, size_t prio)
{
    enum aushape_rc rc;
    struct aushape_gbnode *node;

    assert(aushape_gbtree_is_valid(gbtree));

    AUSHAPE_GUARD(aushape_gbtree_node_put(gbtree, index, prio, &node));

    /* Set the node data */
    node->type = AUSHAPE_GBNODE_TYPE_TEXT;
    node->pos = gbtree->tail;
    node->len = gbtree->text.len - gbtree->tail;

    /* Move the tail */
    gbtree->tail = gbtree->text.len;

    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

enum aushape_rc
aushape_gbtree_node_add_text(struct aushape_gbtree *gbtree, size_t prio)
{
    assert(aushape_gbtree_is_valid(gbtree));
    return aushape_gbtree_node_put_text(gbtree,
                                        aushape_garr_get_len(&gbtree->nodes),
                                        prio);
}

enum aushape_rc
aushape_gbtree_node_put_tree(struct aushape_gbtree *gbtree,
                             size_t index,
                             size_t prio,
                             struct aushape_gbtree *node_tree)
{
    enum aushape_rc rc;
    struct aushape_gbnode *node;

    assert(aushape_gbtree_is_valid(gbtree));
    assert(aushape_gbtree_is_valid(node_tree));

    AUSHAPE_GUARD(aushape_gbtree_node_put(gbtree, index, prio, &node));

    node->type = AUSHAPE_GBNODE_TYPE_TREE;
    node->tree = node_tree;

    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

enum aushape_rc
aushape_gbtree_node_add_tree(struct aushape_gbtree *gbtree,
                             size_t prio,
                             struct aushape_gbtree *node_tree)
{
    assert(aushape_gbtree_is_valid(gbtree));
    assert(aushape_gbtree_is_valid(node_tree));
    return aushape_gbtree_node_put_tree(gbtree,
                                        aushape_garr_get_len(&gbtree->nodes),
                                        prio,
                                        node_tree);
}

/**
 * Get the (cached) length of a given priority level contents in a growing
 * buffer tree.
 *
 * @param gbtree    The growing buffer tree to get the length of a priority
 *                  level contents for.
 * @param cached    True if cached tree lengths should be used,
 *                  false if buffer lengths should be calculated and cached.
 * @param prio      The priority level to get the contents length of.
 *
 * @return The length of contents on the specified priority level.
 */
static size_t
aushape_gbtree_prio_get_len(struct aushape_gbtree *gbtree,
                            bool cached,
                            size_t prio)
{
    struct aushape_garr *nodes = &gbtree->nodes;
    struct aushape_garr *prios = &gbtree->prios;
    size_t len = 0;
    size_t head_index;
    size_t index;
    struct aushape_gbnode *node;

    assert(aushape_gbtree_is_valid(gbtree));
    assert(prio < aushape_garr_get_len(prios));

    head_index = *(size_t *)aushape_garr_get(prios, prio);
    /* If priority is initialized */
    if (~head_index != 0) {
        index = head_index;
        do {
            node = aushape_garr_get(nodes, index);
            len += aushape_gbnode_get_len(node, cached);
            index = node->next_index;
        } while (index != head_index);
    }

    return len;
}

/**
 * Void all nodes for a given priority level in a growing buffer tree.
 *
 * @param gbtree The growing buffer tree to void a priority level in.
 * @param prio  The priority level to void.
 */
static void
aushape_gbtree_prio_void(struct aushape_gbtree *gbtree, size_t prio)
{
    struct aushape_garr *nodes = &gbtree->nodes;
    struct aushape_garr *prios = &gbtree->prios;
    size_t head_index;

    assert(aushape_gbtree_is_valid(gbtree));
    assert(prio < aushape_garr_get_len(prios));

    head_index = *(size_t *)aushape_garr_get(prios, prio);
    /* If priority is initialized */
    if (~head_index != 0) {
        struct aushape_gbnode *node;
        size_t index = head_index;
        do {
            node = aushape_garr_get(nodes, index);
            node->type = AUSHAPE_GBNODE_TYPE_VOID;
            index = node->next_index;
        } while (index != head_index);
        aushape_garr_set_byte_span(prios, prio, 0xff, 1);
    }
}

/**
 * Trim each node for a given priority level in a growing buffer tree
 * proportionally, to fit the specified length. Updates the cached lengths of
 * underlying buffers.
 *
 * @param gbtree        The growing buffer tree to trim a priority level in.
 * @param atomic_cached True if cached tree atomicity should be used,
 *                      false if atomicity should be determined and cached.
 * @param len_cached    True if cached tree lengths should be used, false if
 *                      tree lengths should be calculated and cached.
 * @param prio          The priority level to trim.
 * @param len           The length to trim the level to.
 *
 * @return The resulting length of the priority level contents. Can be bigger
 *         than the requested length if the level ended up being atomic.
 */
static size_t
aushape_gbtree_prio_trim(struct aushape_gbtree *gbtree,
                         bool atomic_cached,
                         bool len_cached,
                         size_t prio,
                         size_t len)
{
    struct aushape_garr *nodes = &gbtree->nodes;
    struct aushape_garr *prios = &gbtree->prios;
    size_t head_index;
    struct aushape_gbnode *node;
    size_t prio_len;
    size_t prio_len_atomic;
    size_t prio_len_non_atomic;
    size_t len_non_atomic;

    assert(aushape_gbtree_is_valid(gbtree));
    assert(prio < aushape_garr_get_len(prios));

    while (true) {
        /*
         * Calculate atomic, non-atomic, and total lengths
         */
        prio_len_atomic = 0;
        prio_len_non_atomic = 0;
        /* For each node */
        head_index = *(size_t *)aushape_garr_get(prios, prio);
        if (~head_index != 0) {
            size_t index = head_index;
            size_t node_len;
            do {
                node = aushape_garr_get(nodes, index);
                node_len = aushape_gbnode_get_len(node, len_cached);
                if (aushape_gbnode_is_atomic(node, atomic_cached)) {
                    prio_len_atomic += node_len;
                } else {
                    prio_len_non_atomic += node_len;
                }
                index = node->next_index;
            } while (index != head_index);
        }
        prio_len = prio_len_atomic + prio_len_non_atomic;

        /* We're cached now */
        len_cached = true;
        atomic_cached = true;

        /* If we can't or don't have to trim anymore */
        if (prio_len_non_atomic == 0 || prio_len <= len) {
            break;
        }

        /*
         * Calculate the length we have to trim remaining non-atomic nodes to.
         * This can turn out to be zero.
         */
        len_non_atomic = (prio_len_atomic < len)
                            ? (len - prio_len_atomic)
                            : 0;

        /*
         * Try to trim as much as possible
         */
        /* For each node */
        head_index = *(size_t *)aushape_garr_get(prios, prio);
        if (~head_index != 0) {
            size_t index = head_index;
            do {
                node = aushape_garr_get(nodes, index);
                /* If the node is not (cached) atomic */
                if (!aushape_gbnode_is_atomic(node, atomic_cached)) {
                    size_t orig_node_len;
                    size_t req_node_len;
                    orig_node_len = aushape_gbnode_get_len(node, len_cached);
                    req_node_len = orig_node_len * len_non_atomic /
                                        prio_len_non_atomic;
                    aushape_gbnode_trim(node, atomic_cached, len_cached,
                                        req_node_len);
                }
                index = node->next_index;
            } while (index != head_index);
        }
    }

    return prio_len;
}

size_t
aushape_gbtree_trim(struct aushape_gbtree *gbtree,
                    bool atomic_cached,
                    bool len_cached,
                    size_t len)
{
    struct aushape_garr *prios = &gbtree->prios;
    size_t tree_len = 0;
    size_t prio;
    size_t prio_len;
    size_t next_tree_len;
    assert(aushape_gbtree_is_valid(gbtree));

    /*
     * Calculate length of all priority levels that fit,
     * and find the first one that doesn't.
     */
    for (prio = 0;
         prio < aushape_garr_get_len(prios);
         prio++) {
        prio_len = aushape_gbtree_prio_get_len(gbtree, len_cached, prio);
        next_tree_len = tree_len + prio_len;
        if (next_tree_len > len) {
            break;
        }
        tree_len = next_tree_len;
    }

    /* Trim the first non-fitting priority level, if any */
    if (prio < aushape_garr_get_len(prios)) {
        /* Trim using lengths cached above, or before */
        next_tree_len = tree_len + 
                        aushape_gbtree_prio_trim(gbtree, atomic_cached, true,
                                                 prio,
                                                 prio_len - (next_tree_len - len));
        /* If trimmed successfully, or if it was non-removable level zero */
        if (next_tree_len <= len || prio == 0) {
            /* Keep this priority level */
            tree_len = next_tree_len;
            prio++;
        }
    }
        
    /* Void all remaining priority levels */
    for (; prio < aushape_garr_get_len(prios); prio++) {
        aushape_gbtree_prio_void(gbtree, prio);
    }

    gbtree->atomic = (tree_len > len);
    gbtree->len = tree_len;
    return gbtree->len;
}


enum aushape_rc
aushape_gbtree_render(struct aushape_gbtree *gbtree, struct aushape_gbuf *gbuf)
{
    struct aushape_garr *nodes = &gbtree->nodes;
    enum aushape_rc rc;
    size_t i;

    assert(aushape_gbtree_is_valid(gbtree));
    assert(aushape_gbuf_is_valid(gbuf));

    for (i = 0; i < aushape_garr_get_len(nodes); i++) {
        AUSHAPE_GUARD(aushape_gbnode_render(
                            aushape_garr_get(nodes, i), gbuf));
    }

    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

static enum aushape_rc
aushape_gbtree_prio_render_dump(const struct aushape_gbtree *gbtree,
                                struct aushape_gbuf *gbuf,
                                size_t prio,
                                const struct aushape_format *format,
                                size_t level,
                                bool first)
{
    const struct aushape_garr *nodes = &gbtree->nodes;
    const struct aushape_garr *prios = &gbtree->prios;
    enum aushape_rc rc;
    size_t head_index;
    size_t index;
    const struct aushape_gbnode *node;
    size_t l = level;

    assert(aushape_gbuf_is_valid(gbuf));
    assert(aushape_gbtree_is_valid(gbtree));
    assert(prio < aushape_garr_get_len(prios));
    assert(aushape_format_is_valid(format));

    if (format->lang == AUSHAPE_LANG_JSON && !first) {
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
    }
    head_index = *(size_t *)aushape_garr_const_get(prios, prio);
    /* If priority is initialized */
    if (~head_index == 0) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
        if (format->lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<prio/>"));
        } else if (format->lang == AUSHAPE_LANG_JSON) {
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "[]"));
        }
    } else {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
        if (format->lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<prio>"));
        } else if (format->lang == AUSHAPE_LANG_JSON) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '['));
        }
        l++;

        index = head_index;
        do {
            node = aushape_garr_const_get(nodes, index);
            AUSHAPE_GUARD(aushape_gbnode_render_dump(node, gbuf, format, l,
                                                     index == head_index));
            index = node->next_index;
        } while (index != head_index);

        l--;
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, format, l));
        if (format->lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</prio>"));
        } else if (format->lang == AUSHAPE_LANG_JSON) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ']'));
        }
    }

    assert(l == level);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

enum aushape_rc
aushape_gbtree_render_dump(const struct aushape_gbtree *gbtree,
                           struct aushape_gbuf *gbuf,
                           const struct aushape_format *format,
                           size_t level,
                           bool first)
{
    enum aushape_rc rc;
    size_t i;
    size_t l = level;
    const struct aushape_garr *nodes = &gbtree->nodes;
    const struct aushape_garr *prios = &gbtree->prios;
    const struct aushape_gbnode *node;

    assert(aushape_gbuf_is_valid(gbuf));
    assert(aushape_gbtree_is_valid(gbtree));
    assert(aushape_format_is_valid(format));

    if (format->lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<tree>"));
        l++;
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<nodes>"));
    } else if (format->lang == AUSHAPE_LANG_JSON) {
        if (!first) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
        }
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '{'));
        l++;
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"type\":\"tree\""));

        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"nodes\":["));
    }
    l++;

    for (i = 0; i < aushape_garr_get_len(nodes); i++) {
        node = aushape_garr_const_get(nodes, i);
        AUSHAPE_GUARD(aushape_gbnode_render_dump(node, gbuf,
                                                 format, l, i == 0));
    }

    l--;
    AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, format, l));
    if (format->lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</nodes>"));
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<prios>"));
    } else if (format->lang == AUSHAPE_LANG_JSON) {
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ']'));
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"prios\":["));
    }
    l++;

    for (i = 0; i < aushape_garr_get_len(prios); i++) {
        AUSHAPE_GUARD(aushape_gbtree_prio_render_dump(gbtree, gbuf, i,
                                                      format, l, i == 0));
    }

    l--;
    AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, format, l));
    if (format->lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</prios>"));
        l--;
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</tree>"));
    } else if (format->lang == AUSHAPE_LANG_JSON) {
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ']'));
        l--;
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '}'));
    }

    assert(l == level);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

enum aushape_rc
aushape_gbtree_print_dump_to_fd(const struct aushape_gbtree *gbtree,
                                int fd,
                                enum aushape_lang lang)
{
    enum aushape_rc rc;
    struct aushape_format format;
    struct aushape_gbuf gbuf;

    assert(fd >= 0);
    assert(aushape_gbtree_is_valid(gbtree));
    assert(aushape_lang_is_valid(lang));

    format = (struct aushape_format){
                    .lang = lang,
                    .fold_level = SIZE_MAX,
                    .nest_indent = 4,
                    .max_event_size = AUSHAPE_FORMAT_MIN_MAX_EVENT_SIZE};

    aushape_gbuf_init(&gbuf, 4096);

    AUSHAPE_GUARD(aushape_gbtree_render_dump(gbtree, &gbuf,
                                             &format, 0, true));
    /* TODO Handle errors */
    write(fd, gbuf.ptr, gbuf.len);

    rc = AUSHAPE_RC_OK;
cleanup:
    aushape_gbuf_cleanup(&gbuf);
    return rc;
}

enum aushape_rc
aushape_gbtree_print_dump_to_file(const struct aushape_gbtree *gbtree,
                                  const char *filename,
                                  enum aushape_lang lang)
{
    int fd;

    assert(filename != NULL);
    assert(aushape_gbtree_is_valid(gbtree));
    assert(aushape_lang_is_valid(lang));

    /* TODO Handle errors */
    fd = open(filename,
              O_CREAT | O_TRUNC | O_WRONLY,
              S_IRUSR | S_IWUSR |
              S_IRGRP | S_IWGRP |
              S_IROTH | S_IWOTH);

    return aushape_gbtree_print_dump_to_fd(gbtree, fd, lang);
}

