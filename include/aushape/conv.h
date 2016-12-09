/**
 * @brief An aushape raw audit log converter
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

#ifndef _AUSHAPE_CONV_H
#define _AUSHAPE_CONV_H

#include <aushape/output.h>
#include <aushape/format.h>
#include <aushape/rc.h>
#include <stddef.h>
#include <stdbool.h>

/** Converter state */
struct aushape_conv;

/**
 * Check if a converter pointer is valid.
 *
 * @param conv  The converter to check.
 *
 * @return True if the converter is valid.
 */
bool aushape_conv_is_valid(const struct aushape_conv *conv);

/**
 * Create (allocate and initialize) a converter.
 *
 * @param pconv             Location for the created converter pointer.
 *                          Not modified in case of error. Cannot be NULL.
 * @param format            The output format to use.
 * @param output            The output to write to.
 * @param output_owned      True if the output should be destroyed when
 *                          converter is destroyed.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK                   - created successfully,
 *          AUSHAPE_RC_INVALID_ARGS         - invalid arguments received,
 *          AUSHAPE_RC_NOMEM                - memory allocation failed.
 */
enum aushape_rc aushape_conv_create(struct aushape_conv **pconv,
                                    const struct aushape_format *format,
                                    struct aushape_output *output,
                                    bool output_owned);

/**
 * Begin converter document output. Must be called once before
 * aushape_conv_input, aushape_conv_flush, and aushape_conv_end. Has effect
 * only if converter was created with format->events_per_doc == SSIZE_MAX.
 *
 * @param pconv         Converter to start the document output with.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK                   - document started successfully,
 *          AUSHAPE_RC_INVALID_ARGS         - invalid arguments received,
 *          AUSHAPE_RC_INVALID_STATE        - called after document was
 *                                            started with aushape_conv_begin,
 *                                            but not finished with
 *                                            aushape_conv_end,
 *          AUSHAPE_RC_NOMEM                - memory allocation failed,
 *          AUSHAPE_RC_OUTPUT_WRITE_FAILED  - output write failed.
 */
enum aushape_rc aushape_conv_begin(struct aushape_conv *conv);

/**
 * Provide a piece of raw audit log input to a converter.
 * Can only be called after aushape_conv_begin and before aushape_conv_end.
 *
 * @param conv  The converter to provide input to.
 * @param ptr   The pointer to the log piece. Can be NULL, if len is 0.
 * @param len   The length of the log piece.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK                   - piece inputted successfully,
 *          AUSHAPE_RC_INVALID_ARGS         - invalid arguments received,
 *          AUSHAPE_RC_INVALID_STATE        - called before starting document
 *                                            output with aushape_conv_begin,
 *                                            or after ending it with
 *                                            aushape_conv_end with
 *                                            format->events_per_doc ==
 *                                            SSIZE_MAX,
 *          AUSHAPE_RC_NOMEM                - memory allocation failed,
 *          AUSHAPE_RC_AUPARSE_FAILED       - an auparse call failed,
 *          AUSHAPE_RC_INVALID_EXECVE       - invalid execve record sequence
 *                                            encountered,
 *          AUSHAPE_RC_OUTPUT_WRITE_FAILED  - output write failed.
 */
enum aushape_rc aushape_conv_input(struct aushape_conv *conv,
                                   const char *ptr,
                                   size_t len);

/**
 * Process any raw log input buffered in a converter. To be called at the end
 * of a log to make sure everything was processed. Can only be called after
 * aushape_conv_begin and before aushape_conv_end.
 *
 * @param conv  The converter to flush.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK                   - flushed successfully,
 *          AUSHAPE_RC_INVALID_ARGS         - invalid arguments received,
 *          AUSHAPE_RC_INVALID_STATE        - called before starting document
 *                                            output with aushape_conv_begin,
 *                                            or after ending it with
 *                                            aushape_conv_end with
 *                                            format->events_per_doc ==
 *                                            SSIZE_MAX,
 *          AUSHAPE_RC_NOMEM                - memory allocation failed,
 *          AUSHAPE_RC_AUPARSE_FAILED       - an auparse call failed,
 *          AUSHAPE_RC_INVALID_EXECVE       - invalid execve record sequence
 *                                            encountered,
 *          AUSHAPE_RC_OUTPUT_WRITE_FAILED  - output write failed.
 */
enum aushape_rc aushape_conv_flush(struct aushape_conv *conv);

/**
 * End converter document output. Can only be called after aushape_conv_begin.
 * Has effect only if converter was created with format->events_per_doc != 0
 * and a document was started.
 *
 * @param pconv         Converter to start the document output with.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK                   - document finished successfully,
 *          AUSHAPE_RC_INVALID_ARGS         - invalid arguments received,
 *          AUSHAPE_RC_INVALID_STATE        - called before starting document
 *                                            output with aushape_conv_begin,
 *                                            if format->events_per_doc ==
 *                                            SSIZE_MAX,
 *          AUSHAPE_RC_NOMEM                - memory allocation failed,
 *          AUSHAPE_RC_OUTPUT_WRITE_FAILED  - output write failed.
 */
enum aushape_rc aushape_conv_end(struct aushape_conv *conv);

/**
 * Destroy (cleanup and free) a converter. If converter was created with
 * output_owned true, then the supplied output is destroyed as well.
 *
 * @param conv  The converter to destroy. Can be NULL.
 *              Must be valid, if not NULL.
 */
void aushape_conv_destroy(struct aushape_conv *conv);

#endif /* _AUSHAPE_CONV_H */
