/**
 * @brief (Exponentially) growing buffer
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

#ifndef _AUSHAPE_GBUF_H
#define _AUSHAPE_GBUF_H

#include <aushape/format.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

/** Minimum growing buffer size */
#define AUSHAPE_GBUF_SIZE_MIN    4096

/** An (exponentially) growing buffer */
struct aushape_gbuf {
    char   *ptr;    /**< Pointer to the buffer memory */
    size_t  size;   /**< Buffer size */
    size_t  len;    /**< Buffer contents length */
};

/**
 * Check if a growing buffer is valid.
 *
 * @param gbuf  The growing buffer to check.
 *
 * @return True if the growing buffer is valid, false otherwise.
 */
extern bool aushape_gbuf_is_valid(const struct aushape_gbuf *gbuf);

/**
 * Initialize a growing buffer.
 *
 * @param gbuf  The growing buffer to initialize.
 */
extern void aushape_gbuf_init(struct aushape_gbuf *gbuf);

/**
 * Cleanup a growing buffer.
 *
 * @param gbuf  The growing buffer to cleanup.
 */
extern void aushape_gbuf_cleanup(struct aushape_gbuf *gbuf);

/**
 * Empty a growing buffer.
 *
 * @param gbuf  The growing buffer to empty.
 */
extern void aushape_gbuf_empty(struct aushape_gbuf *gbuf);

/**
 * Check if a growing buffer is empty.
 *
 * @param gbuf  The growing buffer to check.
 *
 * @return True if the buffer is empty, false otherwise.
 */
extern bool aushape_gbuf_is_empty(const struct aushape_gbuf *gbuf);

/**
 * Make sure a growing buffer can accommodate contents of the specified size in
 * bytes. If the allocated buffer memory wasn't enough to accommodate the
 * specified size, then the memory is reallocated appropriately.
 *
 * @param gbuf  The growing buffer to accommodate the contents in.
 * @param len   The size of the contents in bytes.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - accommodated successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_accomodate(struct aushape_gbuf *gbuf,
                                               size_t len);

/**
 * Add a character to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the character to.
 * @param c     The character to add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_char(struct aushape_gbuf *gbuf,
                                             char c);

/**
 * Add a span filled with a character to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the span to.
 * @param c     The character to fill the span with.
 * @param l     The length of the span to fill.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_span(struct aushape_gbuf *gbuf,
                                             int c, size_t l);

/**
 * Add the contents of an abstract buffer to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the abstract buffer to.
 * @param ptr   The pointer to the buffer to add.
 * @param len   The length of the buffer to add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_buf(struct aushape_gbuf *gbuf,
                                            const void *ptr, size_t len);

/**
 * Add the contents of an abstract buffer to a growing buffer, lowercasing all
 * characters.
 *
 * @param gbuf  The growing buffer to add the abstract buffer to.
 * @param ptr   The pointer to the buffer to add.
 * @param len   The length of the buffer to add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_buf_lowercase(struct aushape_gbuf *gbuf,
                                                      const void *ptr,
                                                      size_t len);

/**
 * Add a string to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the string to.
 * @param str   The string to add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_str(struct aushape_gbuf *gbuf,
                                            const char *str);

/**
 * Add a string to a growing buffer, lowercasing all characters.
 *
 * @param gbuf  The growing buffer to add the string to.
 * @param str   The string to lowercase and add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_str_lowercase(
                                            struct aushape_gbuf *gbuf,
                                            const char *str);

/**
 * Add a printf-formatted string to a growing buffer,
 * with arguments in a va_list.
 *
 * @param gbuf  The growing buffer to add the formatted string to.
 * @param fmt   The format string.
 * @param ap    The format arguments.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_vfmt(struct aushape_gbuf *gbuf,
                                             const char *fmt, va_list ap);

/**
 * Add a printf-formatted string to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the formatted string to.
 * @param fmt   The format string.
 * @param ...   The format arguments.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_fmt(struct aushape_gbuf *gbuf,
                                            const char *fmt, ...)
                                    __attribute__((format(printf, 2, 3)));

/**
 * Add leading whitespace for an opening of a nested block to a growing
 * buffer, according to an output format. If the specified nesting level is
 * not zero and is not folded, then adds a newline, if it's just not folded,
 * then adds indenting space, otherwise adds nothing.
 *
 * @param gbuf      The growing buffer to add the whitespace to.
 * @param format    The format according to which the whitespace should be
 *                  added.
 * @param level     The block's nesting level.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_space_opening(
                                        struct aushape_gbuf *gbuf,
                                        const struct aushape_format *format,
                                        size_t level);

/**
 * Add leading whitespace for a closing of a nested block to a growing
 * buffer, according to an output format. If the nesting level above the
 * specified one is not folded, then adds a newline and indenting space,
 * otherwise adds nothing.
 *
 * @param gbuf      The growing buffer to add the whitespace to.
 * @param format    The format according to which the whitespace should be
 *                  added.
 * @param level     The block's nesting level.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_space_closing(
                                        struct aushape_gbuf *gbuf,
                                        const struct aushape_format *format,
                                        size_t level);

/**
 * Add the contents of an abstract buffer to a growing buffer,
 * escaped as XML text.
 *
 * @param gbuf      The growing buffer to add the escaped buffer contents to.
 * @param ptr       The pointer to the buffer to escape and add.
 * @param len       The length of the buffer to escape and add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_buf_xml(struct aushape_gbuf *gbuf,
                                                const void *ptr, size_t len);

/**
 * Add a string to a growing buffer, escaped as XML text.
 *
 * @param gbuf      The growing buffer to add the escaped string to.
 * @param str       The string to escape and add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_str_xml(struct aushape_gbuf *gbuf,
                                                const char *str);

/**
 * Add the contents of an abstract buffer to a growing buffer, escaped as a
 * JSON string value.
 *
 * @param gbuf      The growing buffer to add the escaped buffer contents to.
 * @param ptr       The pointer to the buffer to escape and add.
 * @param len       The length of the buffer to escape and add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_buf_json(struct aushape_gbuf *gbuf,
                                                 const void *ptr, size_t len);

/**
 * Add a string to a growing buffer, escaped as a JSON string value.
 *
 * @param gbuf      The growing buffer to add the escaped string to.
 * @param str       The string to escape and add.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK       - added successfully,
 *          AUSHAPE_RC_NOMEM    - failed to allocate memory.
 */
extern enum aushape_rc aushape_gbuf_add_str_json(struct aushape_gbuf *gbuf,
                                                 const char *str);

#endif /* _AUSHAPE_GBUF_H */
