/**
 * @brief An (exponentially) growing buffer tree with trimming support.
 */
/*
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

#ifndef _AUSHAPE_GBTREE_H
#define _AUSHAPE_GBTREE_H

#include <aushape/gbuf.h>
#include <aushape/garr.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * An (exponentially) growing buffer tree.
 * Can be cast to struct aushape_gbuf to represent the text buffer.
 */
struct aushape_gbtree {
    /**
     * Text buffer.
     * Referred to by text nodes.
     */
    struct aushape_gbuf     text;
    /**
     * Position->node map.
     * Lower numbers are closer to the beginning.
     */
    struct aushape_garr     nodes;
    /**
     * Priority->node index map.
     * SIZE_MAX means no index.
     * Lower numbers are higher priorities.
     */
    struct aushape_garr     prios;
    /**
     * Cached atomic status of the buffer content.
     * If true, then this tree cannot be trimmed,
     * false otherwise.
     * Used during trimming.
     */
    size_t                  atomic;
    /**
     * Cached length of the buffer content.
     * Used during trimming.
     */
    size_t                  len;
    /**
     * End of the last text node added, in the text buffer,
     * or zero if no text nodes were added yet
     */
    size_t                  tail;
};

/**
 * Check if a growing buffer tree is valid.
 *
 * @param gbtree The growing buffer tree to check.
 *
 * @return True if the growing buffer tree is valid, false otherwise.
 */
extern bool aushape_gbtree_is_valid(const struct aushape_gbtree *gbtree);

/**
 * Initialize a growing buffer tree.
 *
 * @param gbtree    The growing buffer tree to initialize.
 * @param text_min  Initial number of bytes to allocate for the text buffer.
 *                  Cannot be zero.
 * @param node_min  Initial number of nodes to allocate.
 *                  Cannot be zero.
 * @param prio_min  Initial number of priorities to allocate.
 *                  Cannot be zero.
 */
extern void aushape_gbtree_init(struct aushape_gbtree *gbtree,
                                size_t text_min,
                                size_t node_min,
                                size_t prio_min);

/**
 * Cleanup a growing buffer tree.
 *
 * @param gbtree    The growing buffer tree to cleanup.
 */
extern void aushape_gbtree_cleanup(struct aushape_gbtree *gbtree);

/**
 * Empty a growing buffer tree.
 *
 * @param gbtree The growing buffer tree to empty.
 */
extern void aushape_gbtree_empty(struct aushape_gbtree *gbtree);

/**
 * Check if a growing buffer tree is empty,
 * i.e. if it renders to an empty string.
 *
 * @param gbtree    The growing buffer tree to check.
 *
 * @return True if the tree is empty, false otherwise.
 */
extern bool aushape_gbtree_is_empty(const struct aushape_gbtree *gbtree);

/**
 * Check if a growing buffer tree is solid (or continuous), i.e. if there are
 * no missing nodes.
 *
 * @param gbtree    The growing buffer tree to check.
 *
 * @return True if the tree is solid, false otherwise.
 */
extern bool aushape_gbtree_is_solid(const struct aushape_gbtree *gbtree);

/**
 * Check if a growing buffer tree is atomic, i.e. if it can be trimmed.
 *
 * @param gbtree    The growing buffer tree to check.
 * @param cached    True if the cached atomic status should be used,
 *                  false if it should be determined and cached.
 *
 * @return True if the tree is atomic, false otherwise.
 */
extern bool aushape_gbtree_is_atomic(struct aushape_gbtree *gbtree,
                                     bool cached);

/**
 * Get the (cached) length of a growing buffer tree content.
 *
 * @param gbtree    The growing buffer tree to get the length of.
 * @param cached    True if cached tree lengths should be used,
 *                  false if tree lengths should be calculated and cached.
 *
 * @return The length of the growing buffer tree contents.
 */
extern size_t aushape_gbtree_get_len(struct aushape_gbtree *gbtree,
                                     bool cached);

/**
 * Get the number of nodes (including void ones) in a growing buffer tree.
 *
 * @param gbtree    The growing buffer tree to get the length of.
 *
 * @return The number of nodes in the growing buffer tree.
 */
static inline size_t
aushape_gbtree_get_node_num(struct aushape_gbtree *gbtree)
{
    assert(aushape_gbtree_is_valid(gbtree));
    return aushape_garr_get_len(&gbtree->nodes);
}

/**
 * Check if a growing buffer tree contains a node at a specified index.
 *
 * @param gbtree    The growing buffer tree to check in.
 * @param index     The index to check at.
 *
 * @return True if a node exists in the tree at the specified index,
 *         false otherwise.
 */
extern bool aushape_gbtree_node_exists(const struct aushape_gbtree *gbtree,
                                       size_t index);

/**
 * Void a specified node, removing it from future rendered output.
 *
 * @param gbtree    The growing buffer tree to void the node in.
 * @param index     The index of the node to void.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - node voided successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbtree_node_void(struct aushape_gbtree *gbtree,
                                                size_t index);

/**
 * Put a new text node to a specified position in a growing buffer tree, with
 * its contents being the text added to the text buffer since the
 * initialization, or the last text node added. Replaces existing node.
 *
 * @param gbtree    The growing buffer tree to add the text node to.
 * @param index     The index to put the new node at.
 * @param prio      The priority to assign to the added node.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - node added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbtree_node_put_text(
                                        struct aushape_gbtree *gbtree,
                                        size_t index,
                                        size_t prio);

/**
 * Add a new text node to the end of a growing buffer tree, with its contents
 * being the text added to the text buffer since the initialization, or the
 * last text node added.
 *
 * @param gbtree    The growing buffer tree to add the text node to.
 * @param prio      The priority to assign to the added node.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - node added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbtree_node_add_text(
                                        struct aushape_gbtree *gbtree,
                                        size_t prio);

/**
 * Put a new tree node to a specified position in a growing buffer tree, with
 * its contents being another growing buffer tree. Replaces existing node.
 *
 * @param gbtree    The growing buffer tree to add the text node to.
 * @param index     The index in the node array where the new node should be put.
 * @param prio      The priority to assign to the added node.
 * @param node_tree The growing buffer tree to add as the new node's contents.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - node added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbtree_node_put_tree(
                                        struct aushape_gbtree *gbtree,
                                        size_t index,
                                        size_t prio,
                                        struct aushape_gbtree *node_tree);

/**
 * Add a new tree node to the end of a growing buffer tree, with its contents
 * being another growing buffer tree.
 *
 * @param gbtree    The growing buffer tree to add the text node to.
 * @param prio      The priority to assign to the added node.
 * @param node_tree The growing buffer tree to add as the new node's contents.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - node added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbtree_node_add_tree(
                                        struct aushape_gbtree *gbtree,
                                        size_t prio,
                                        struct aushape_gbtree *node_tree);

/**
 * Trim a growing buffer tree to a specified length by voiding nodes of the
 * lowest possible priority until the total contents fits. Items of the same
 * priority are trimmed and voided together.
 *
 * @param gbtree        The growing buffer tree to trim.
 * @param atomic_cached True if cached tree atomicity should be used,
 *                      false if atomicity should be determined and cached.
 * @param len_cached    True if cached tree lengths should be used, false if
 *                      tree lengths should be calculated and cached.
 * @param len           The length to trim the buffer to.
 *
 * @return The resulting length of the buffer contents. Can be bigger than the
 *         requested length if the tree ended up being atomic.
 */
extern size_t aushape_gbtree_trim(struct aushape_gbtree *gbtree,
                                  bool atomic_cached,
                                  bool len_cached,
                                  size_t len);

/**
 * Output the contents of a growing buffer tree to a growing buffer in the
 * node order, with void nodes omitted. In short, "render" the buffer.
 *
 * @param gbtree The growing buffer tree to render.
 * @param gbuf  The growing buffer to render to.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - rendered successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbtree_render(struct aushape_gbtree *gbtree,
                                             struct aushape_gbuf *gbuf);

/**
 * Render a dump of the structure of a growing buffer tree into a growing
 * buffer for debugging.
 *
 * @param gbtree    The growing buffer tree to dump.
 * @param gbuf      The growing buffer to dump into.
 * @param format    The output format to use.
 * @param level     The syntactic nesting level to dump at.
 * @param first     True if the tree is the first one being dumped for a
 *                  container.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - tree added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbtree_render_dump(
                                    const struct aushape_gbtree *gbtree,
                                    struct aushape_gbuf *gbuf,
                                    const struct aushape_format *format,
                                    size_t level,
                                    bool first);

/**
 * Print a dump of the structure of a growing buffer tree into a file
 * descriptor for debugging, in specified language, without leading indent,
 * with 4-space indent per nesting level, and fully unfolded.
 *
 * @param gbtree    The growing buffer tree to dump.
 * @param fd        The descriptor of the file to print to.
 * @param lang      The output language to use.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - tree added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbtree_print_dump_to_fd(
                                    const struct aushape_gbtree *gbtree,
                                    int fd,
                                    enum aushape_lang lang);

/**
 * Print the dump of the structure of a growing buffer tree into a file for
 * debugging, in specified language, without leading indent, with 4-space
 * indent per nesting level, and fully unfolded.
 *
 * @param gbtree    The growing buffer tree to dump.
 * @param filename  The name of the file to print to.
 * @param lang      The output language to use.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - tree added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbtree_print_dump_to_file(
                                    const struct aushape_gbtree *gbtree,
                                    const char *filename,
                                    enum aushape_lang lang);

#endif /* _AUSHAPE_GBTREE_H */
