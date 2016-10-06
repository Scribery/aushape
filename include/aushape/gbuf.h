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
 * Make sure a growing buffer can accomodate contents of the specified size in
 * bytes. If the allocated buffer memory wasn't enough to accomodate the
 * specified size, then the memory is reallocated appropriately.
 *
 * @param gbuf  The growing buffer to accomodate the contents in.
 * @param len   The size of the contents in bytes.
 *
 * @return True if accomodated successfully, false otherwise.
 */
extern bool aushape_gbuf_accomodate(struct aushape_gbuf *gbuf, size_t len);

/**
 * Add a character to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the character to.
 * @param c     The character to add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
extern bool aushape_gbuf_add_char(struct aushape_gbuf *gbuf, char c);

/**
 * Add the contents of an abstract buffer to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the abstract buffer to.
 * @param ptr   The pointer to the buffer to add.
 * @param len   The length of the buffer to add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
extern bool aushape_gbuf_add_buf(struct aushape_gbuf *gbuf,
                                 const void *ptr, size_t len);

/**
 * Add the contents of an abstract buffer to a growing buffer, lowercasing all
 * characters.
 *
 * @param gbuf  The growing buffer to add the abstract buffer to.
 * @param ptr   The pointer to the buffer to add.
 * @param len   The length of the buffer to add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
extern bool aushape_gbuf_add_buf_lowercase(struct aushape_gbuf *gbuf,
                                           const void *ptr, size_t len);

/**
 * Add a string to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the string to.
 * @param str   The string to add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
extern bool aushape_gbuf_add_str(struct aushape_gbuf *gbuf, const char *str);

/**
 * Add a string to a growing buffer, lowercasing all characters.
 *
 * @param gbuf  The growing buffer to add the string to.
 * @param str   The string to lowercase and add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
extern bool aushape_gbuf_add_str_lowercase(struct aushape_gbuf *gbuf,
                                           const char *str);

/**
 * Add a printf-formatted string to a growing buffer,
 * with arguments in a va_list.
 *
 * @param gbuf  The growing buffer to add the formatted string to.
 * @param fmt   The format string.
 * @param ap    The format arguments.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
extern bool aushape_gbuf_add_vfmt(struct aushape_gbuf *gbuf,
                                  const char *fmt, va_list ap);

/**
 * Add a printf-formatted string to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the formatted string to.
 * @param fmt   The format string.
 * @param ...   The format arguments.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
extern bool aushape_gbuf_add_fmt(struct aushape_gbuf *gbuf,
                                 const char *fmt, ...);

/**
 * Add the contents of an abstract buffer to a growing buffer, escaped as a
 * double-quoted XML attribute value.
 *
 * @param gbuf      The growing buffer to add the escaped buffer contents to.
 * @param ptr       The pointer to the buffer to escape and add.
 * @param len       The length of the buffer to escape and add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
extern bool aushape_gbuf_add_buf_xml(struct aushape_gbuf *gbuf,
                                     const void *ptr, size_t len);

/**
 * Add a string to a growing buffer, escaped as a double-quoted XML attribute
 * value.
 *
 * @param gbuf      The growing buffer to add the escaped string to.
 * @param str       The string to escape and add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
extern bool aushape_gbuf_add_str_xml(struct aushape_gbuf *gbuf,
                                     const char *str);

/**
 * Add the contents of an abstract buffer to a growing buffer, escaped as a
 * JSON string value.
 *
 * @param gbuf      The growing buffer to add the escaped buffer contents to.
 * @param ptr       The pointer to the buffer to escape and add.
 * @param len       The length of the buffer to escape and add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
extern bool aushape_gbuf_add_buf_json(struct aushape_gbuf *gbuf,
                                      const void *ptr, size_t len);

/**
 * Add a string to a growing buffer, escaped as a JSON string value.
 *
 * @param gbuf      The growing buffer to add the escaped string to.
 * @param str       The string to escape and add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
extern bool aushape_gbuf_add_str_json(struct aushape_gbuf *gbuf,
                                      const char *str);

#endif /* _AUSHAPE_GBUF_H */
