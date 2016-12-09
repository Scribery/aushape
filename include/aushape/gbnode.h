/**
 * @brief A growing buffer node.
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

#ifndef _AUSHAPE_GBNODE_H
#define _AUSHAPE_GBNODE_H

#include <aushape/gbuf.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

/** Forward declaration of an (exponentially) growing buffer tree */
struct aushape_gbtree;

/** Growing buffer node type */
enum aushape_gbnode_type {
    /** Void node (no node in an array) */
    AUSHAPE_GBNODE_TYPE_VOID,
    /** Text node. Represents a piece of the tree's own text */
    AUSHAPE_GBNODE_TYPE_TEXT,
    /** Tree node. Represents the tree itself. */
    AUSHAPE_GBNODE_TYPE_TREE,
    /** Number of node types, not a valid node type */
    AUSHAPE_GBNODE_TYPE_NUM
};

/**
 * A growing buffer node.
 * Zeroed memory constitutes a void node.
 */
struct aushape_gbnode {
    /** Node type */
    enum aushape_gbnode_type    type;
    /** Node priority */
    size_t                      prio;
    /** Index of the previous node with the same priority */
    size_t                      prev_index;
    /** Index of the next node with the same priority */
    size_t                      next_index;
    /**
     * The node owner tree.
     * Must be valid, if node is not void.
     */
    struct aushape_gbtree      *owner;
    /**
     * The growing buffer tree this node refers to.
     * Must be valid, if node type is AUSHAPE_GBNODE_TYPE_TREE.
     */
    struct aushape_gbtree      *tree;
    /**
     * Position of the node text in the owner's text buffer.
     * Cannot point outside the owner's text buffer.
     */
    size_t                      pos;
    /**
     * Length of the node text in the owner's text buffer.
     * Cannot point outside the owner's text buffer.
     */
    size_t                      len;
};

/**
 * Check if a growing buffer node is valid.
 *
 * @param gbnode    The growing buffer node to check.
 *
 * @return True if the node is valid, false otherwise.
 */
extern bool aushape_gbnode_is_valid(const struct aushape_gbnode *gbnode);

/**
 * Check if a growing buffer node is empty,
 * i.e. if it is void, or its tree is empty.
 *
 * @param gbnode    The growing buffer node to check.
 *
 * @return True if the node is empty, false otherwise.
 */
extern bool aushape_gbnode_is_empty(const struct aushape_gbnode *gbnode);

/**
 * Check if a growing buffer node is solid,
 * i.e. if it is not void or its buffer is solid.
 *
 * @param gbnode    The growing buffer node to check.
 *
 * @return True if the node is solid, false otherwise.
 */
extern bool aushape_gbnode_is_solid(const struct aushape_gbnode *gbnode);

/**
 * Check if a growing buffer node is atomic, i.e. if it can be trimmed.
 *
 * @param gbnode    The growing buffer node to check.
 * @param cached    True if the cached atomic status should be used,
 *                  false if it should be determined and cached.
 *
 * @return True if the node is atomic, false otherwise.
 */
extern bool aushape_gbnode_is_atomic(struct aushape_gbnode *gbnode,
                                     bool cached);

/**
 * Get the (cached) length of a growing buffer node content.
 *
 * @param gbnode    The growing buffer node to get the content length of.
 * @param cached    True if cached tree lengths should be used,
 *                  false if tree lengths should be calculated and cached.
 *
 * @return The node content length.
 */
extern size_t aushape_gbnode_get_len(struct aushape_gbnode *gbnode,
                                     bool cached);

/**
 * Trim a node to fit the specified length.
 *
 * @param gbnode        The growing buffer node to trim.
 * @param atomic_cached True if cached tree atomicity should be used,
 *                      false if atomicity should be determined and cached.
 * @param len_cached    True if cached tree lengths should be used, false if
 *                      tree lengths should be calculated and cached.
 * @param len           The length to trim the node to.
 *
 * @return The resulting length of the buffer contents. Can be bigger than the
 *         requested length if the node ended up being atomic.
 */
extern size_t aushape_gbnode_trim(struct aushape_gbnode *gbnode,
                                  bool atomic_cached,
                                  bool len_cached,
                                  size_t len);


/**
 * Output the contents of a growing buffer node to a growing buffer.
 * In short, "render" a node.
 *
 * @param gbnode    The growing buffer node to render.
 * @param gbuf      The growing buffer to render to.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - rendered successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbnode_render(struct aushape_gbnode *gbnode,
                                             struct aushape_gbuf *gbuf);

/**
 * Render a dump of the structure of a growing buffer node into a growing
 * buffer for debugging.
 *
 * @param gbnode    The growing buffer node to dump.
 * @param gbuf      The growing buffer to dump into.
 * @param format    The output format to use.
 * @param level     The syntactic nesting level to dump at.
 * @param first     True if the node is the first one being dumped for a
 *                  container.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - node added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbnode_render_dump(
                                    const struct aushape_gbnode *gbnode,
                                    struct aushape_gbuf *gbuf,
                                    const struct aushape_format *format,
                                    size_t level,
                                    bool first);

/**
 * Print a dump of the structure of a growing buffer node into a file for
 * debugging.
 *
 * @param gbnode    The growing buffer node to dump.
 * @param fd        The descriptor of the file to print to.
 * @param format    The output format to use.
 * @param level     The syntactic nesting level to dump at.
 * @param first     True if the node is the first one being dumped for a
 *                  container.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - node added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbnode_print_dump_to_fd(
                                    const struct aushape_gbnode *gbnode,
                                    int fd,
                                    const struct aushape_format *format,
                                    size_t level,
                                    bool first);

/**
 * Print the dump of the structure of a growing buffer node into a file for
 * debugging, in specified language, without leading indent, with 4-space
 * indent per nesting level, and fully unfolded.
 *
 * @param gbnode    The growing buffer node to dump.
 * @param filename  The name of the file to print to.
 * @param lang      The output language to use.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - node added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbnode_print_dump_to_file(
                                    const struct aushape_gbnode *gbnode,
                                    const char *filename,
                                    enum aushape_lang lang);

#endif /* _AUSHAPE_GBNODE_H */
