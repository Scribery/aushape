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

#include <aushape/format.h>
#include <aushape/rc.h>
#include <unistd.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Output function prototype. Called for each output fragment.
 *
 * @param format    Output format.
 * @param ptr       Pointer to the output fragment.
 * @param len       Length of the output fragment in bytes.
 * @param data      The opaque data pointer, supplied upon converter creation.
 *
 * @return True if the function executed succesfully, false if it failed and
 *         conversion should be terminated.
 */
typedef bool (*aushape_conv_output_fn)(const char *ptr, size_t len,
                                       void *data);

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
 * @param events_per_doc    Amount of events to put into a single output
 *                          document. Zero means "bare" output - no document
 *                          wrapping, and no event separators. One means each
 *                          event is wrapped in a document. SSIZE_MAX means
 *                          all events are put into a single document, even if
 *                          there were none. Negative numbers specify size of
 *                          documents in bytes. Documents are finished when
 *                          size of event text added to it crosses the negated
 *                          number.
 * @param format            The output format to use.
 * @param output_fn         The function to call for each completely formatted
 *                          fragment. Cannot be NULL.
 * @param output_cont       True if the output function accepts continuous
 *                          input, i.e. arbitrary pieces of the output,
 *                          false if it expects complete documents/events.
 * @param output_data       The data to pass to the output function.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK                   - created succesfully,
 *          AUSHAPE_RC_INVALID_ARGS         - invalid arguments received,
 *          AUSHAPE_RC_NOMEM                - memory allocation failed.
 */
enum aushape_rc aushape_conv_create(struct aushape_conv **pconv,
                                    ssize_t events_per_doc,
                                    const struct aushape_format *format,
                                    aushape_conv_output_fn output_fn,
                                    bool output_cont,
                                    void *output_data);

/**
 * Begin converter document output. Must be called once before
 * aushape_conv_input, aushape_conv_flush, and aushape_conv_end. Has effect
 * only if converter was created with events_per_doc == SSIZE_MAX.
 *
 * @param pconv         Converter to start the document output with.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK                   - document started succesfully,
 *          AUSHAPE_RC_INVALID_ARGS         - invalid arguments received,
 *          AUSHAPE_RC_INVALID_STATE        - called after document was
 *                                            started with aushape_conv_begin,
 *                                            but not finished with
 *                                            aushape_conv_end,
 *          AUSHAPE_RC_NOMEM                - memory allocation failed,
 *          AUSHAPE_RC_CONV_OUTPUT_FAILED   - output function failed.
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
 *                                            events_per_doc == SSIZE_MAX,
 *          AUSHAPE_RC_NOMEM                - memory allocation failed,
 *          AUSHAPE_RC_CONV_AUPARSE_FAILED  - an auparse call failed,
 *          AUSHAPE_RC_CONV_INVALID_EXECVE  - invalid execve record sequence
 *                                            encountered,
 *          AUSHAPE_RC_CONV_OUTPUT_FAILED   - output function failed.
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
 *                                            events_per_doc == SSIZE_MAX,
 *          AUSHAPE_RC_NOMEM                - memory allocation failed,
 *          AUSHAPE_RC_CONV_AUPARSE_FAILED  - an auparse call failed,
 *          AUSHAPE_RC_CONV_INVALID_EXECVE  - invalid execve record sequence
 *                                            encountered,
 *          AUSHAPE_RC_CONV_OUTPUT_FAILED   - output function failed.
 */
enum aushape_rc aushape_conv_flush(struct aushape_conv *conv);

/**
 * End converter document output. Can only be called after aushape_conv_begin.
 * Has effect only if converter was created with events_per_doc != 0 and a
 * document was started.
 *
 * @param pconv         Converter to start the document output with.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK                   - document finished succesfully,
 *          AUSHAPE_RC_INVALID_ARGS         - invalid arguments received,
 *          AUSHAPE_RC_INVALID_STATE        - called before starting document
 *                                            output with aushape_conv_begin,
 *                                            if events_per_doc == SSIZE_MAX,
 *          AUSHAPE_RC_NOMEM                - memory allocation failed,
 *          AUSHAPE_RC_CONV_OUTPUT_FAILED   - output function failed.
 */
enum aushape_rc aushape_conv_end(struct aushape_conv *conv);

/**
 * Destroy (cleanup and free) a converter.
 *
 * @param conv  The converter to destroy. Can be NULL.
 *              Must be valid, if not NULL.
 */
void aushape_conv_destroy(struct aushape_conv *conv);

#endif /* _AUSHAPE_CONV_H */
