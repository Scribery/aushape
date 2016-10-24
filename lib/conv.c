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

#include <aushape/conv.h>
#include <aushape/conv/buf.h>
#include <aushape/gbuf.h>
#include <auparse.h>
#include <limits.h>
#include <string.h>

/** Converter */
struct aushape_conv {
    /** Auparse state */
    auparse_state_t            *au;
    /**
     * Amount of events per document.
     * Positive for events, negative for bytes
     */
    ssize_t                     events_per_doc;
    /** Output format */
    struct aushape_format       format;
    /** Output function pointer */
    aushape_conv_output_fn      output_fn;
    /**
     * True if the output function accepts continuous input, i.e. arbitrary
     * pieces of the output, false if it expects complete documents/events.
     */
    bool                        output_cont;
    /** Output function data */
    void                       *output_data;
    /** First conversion failure return code, or OK */
    enum aushape_rc             rc;
    /** Output buffer */
    struct aushape_conv_buf     buf;
    /** True if outputting inside of a document, false otherwise */
    bool                        in_doc;
    /**
     * Amount of events in current document.
     * Events, if events_per_doc is positive, bytes if negative.
     */
    size_t                      events_in_doc;
};

bool
aushape_conv_is_valid(const struct aushape_conv *conv)
{
    return conv != NULL &&
           conv->au != NULL &&
           conv->output_fn != NULL &&
           aushape_conv_buf_is_valid(&conv->buf);
}

/**
 * Handle auparse event callback.
 *
 * @param au    The auparse state corresponding to the reported event.
 * @param type  The event type.
 * @param data  The opaque data passed to auparse_add_calback previously.
 */
static void
aushape_conv_cb(auparse_state_t *au, auparse_cb_event_t type, void *data)
{
    enum aushape_rc rc;
    struct aushape_conv *conv = (struct aushape_conv *)data;

    assert(aushape_conv_is_valid(conv));

    if (type != AUPARSE_CB_EVENT_READY) {
        return;
    }

    /*
     * Output document prologue, if needed
     */
    if (conv->rc == AUSHAPE_RC_OK) {
        if (conv->events_per_doc != 0 && conv->events_per_doc != SSIZE_MAX) {
            if (!conv->in_doc) {
                rc = aushape_conv_buf_add_prologue(&conv->buf, &conv->format, 0);
                if (rc == AUSHAPE_RC_OK) {
                    conv->in_doc = true;
                    if (conv->output_cont) {
                        if (conv->output_fn(conv->buf.gbuf.ptr, conv->buf.gbuf.len,
                                            conv->output_data)) {
                            aushape_conv_buf_empty(&conv->buf);
                        } else {
                            conv->rc = AUSHAPE_RC_CONV_OUTPUT_FAILED;
                        }
                    }
                } else {
                    conv->rc = rc;
                }
            }
        }
    }

    /*
     * Output the event
     */
    if (conv->rc == AUSHAPE_RC_OK) {
        rc = aushape_conv_buf_add_event(&conv->buf, &conv->format,
                                        conv->in_doc ? 1 : 0,
                                        conv->events_in_doc == 0, au);
        if (rc == AUSHAPE_RC_OK) {
            if (conv->events_per_doc > 0) {
                conv->events_in_doc++;
            } else if (conv->events_per_doc < 0) {
                conv->events_in_doc += conv->buf.gbuf.len;
            }
            if (conv->output_cont || conv->events_per_doc == 0) {
                if (conv->output_fn(conv->buf.gbuf.ptr, conv->buf.gbuf.len,
                                     conv->output_data)) {
                    aushape_conv_buf_empty(&conv->buf);
                } else {
                    conv->rc = AUSHAPE_RC_CONV_OUTPUT_FAILED;
                }
            }
        } else {
            conv->rc = rc;
        }
    }

    /*
     * Output document epilogue, if needed
     */
    if (conv->rc == AUSHAPE_RC_OK) {
        if (conv->events_per_doc != 0 && conv->events_per_doc != SSIZE_MAX) {
            assert(conv->in_doc);
            /* If hit the limit */
            if ((conv->events_per_doc > 0 &&
                 conv->events_in_doc >= (size_t)conv->events_per_doc) ||
                (conv->events_per_doc < 0 &&
                 conv->events_in_doc >= (size_t)-conv->events_per_doc)) {
                rc = aushape_conv_buf_add_epilogue(&conv->buf, &conv->format, 0);
                if (rc == AUSHAPE_RC_OK) {
                    if (conv->output_fn(conv->buf.gbuf.ptr, conv->buf.gbuf.len,
                                        conv->output_data)) {
                        aushape_conv_buf_empty(&conv->buf);
                        conv->events_in_doc = 0;
                        conv->in_doc = false;
                    } else {
                        conv->rc = AUSHAPE_RC_CONV_OUTPUT_FAILED;
                    }
                } else {
                    conv->rc = rc;
                }
            }
        }
    }
}

enum aushape_rc
aushape_conv_create(struct aushape_conv **pconv,
                    ssize_t events_per_doc,
                    const struct aushape_format *format,
                    aushape_conv_output_fn output_fn,
                    bool output_cont,
                    void *output_data)
{
    enum aushape_rc rc;
    struct aushape_conv *conv;

    if (pconv == NULL ||
        !aushape_format_is_valid(format) ||
        output_fn == NULL) {
        rc = AUSHAPE_RC_INVALID_ARGS;
        goto cleanup;
    }

    conv = calloc(1, sizeof(*conv));
    if (conv == NULL) {
        rc = AUSHAPE_RC_NOMEM;
        goto cleanup;
    }

    conv->au = auparse_init(AUSOURCE_FEED, NULL);
    if (conv->au == NULL) {
        rc = AUSHAPE_RC_CONV_AUPARSE_FAILED;
        goto cleanup;
    }
    auparse_set_escape_mode(conv->au, AUPARSE_ESC_RAW);
    auparse_add_callback(conv->au, aushape_conv_cb, conv, NULL);

    aushape_conv_buf_init(&conv->buf);
    conv->events_per_doc = events_per_doc;
    conv->format = *format;
    conv->output_fn = output_fn;
    conv->output_cont = output_cont;
    conv->output_data = output_data;

    *pconv = conv;
    conv = NULL;
    assert(aushape_conv_is_valid(*pconv));

    rc = AUSHAPE_RC_OK;

cleanup:
    if (conv != NULL) {
        auparse_destroy(conv->au);
        free(conv);
    }
    return rc;
}

enum aushape_rc
aushape_conv_begin(struct aushape_conv *conv)
{
    if (!aushape_conv_is_valid(conv)) {
        return AUSHAPE_RC_INVALID_ARGS;
    }
    if (conv->events_per_doc != SSIZE_MAX) {
        return AUSHAPE_RC_OK;
    }
    if (conv->in_doc) {
        return AUSHAPE_RC_INVALID_STATE;
    }

    if (conv->rc == AUSHAPE_RC_OK) {
        enum aushape_rc rc;
        rc = aushape_conv_buf_add_prologue(&conv->buf, &conv->format, 0);
        if (rc == AUSHAPE_RC_OK) {
            conv->in_doc = true;
            if (conv->output_cont) {
                if (conv->output_fn(conv->buf.gbuf.ptr, conv->buf.gbuf.len,
                                    conv->output_data)) {
                    aushape_conv_buf_empty(&conv->buf);
                } else {
                    conv->rc = AUSHAPE_RC_CONV_OUTPUT_FAILED;
                }
            }
        } else {
            conv->rc = rc;
        }
        assert(aushape_conv_is_valid(conv));
    }

    return conv->rc;
}

enum aushape_rc
aushape_conv_end(struct aushape_conv *conv)
{
    if (!aushape_conv_is_valid(conv)) {
        return AUSHAPE_RC_INVALID_ARGS;
    }
    if (conv->events_per_doc == 0) {
        return AUSHAPE_RC_OK;
    } else if (!conv->in_doc) {
        if (conv->events_per_doc == SSIZE_MAX) {
            return AUSHAPE_RC_INVALID_STATE;
        } else {
            return AUSHAPE_RC_OK;
        }
    }

    if (conv->rc == AUSHAPE_RC_OK) {
        enum aushape_rc rc;
        rc = aushape_conv_buf_add_epilogue(&conv->buf, &conv->format, 0);
        if (rc == AUSHAPE_RC_OK) {
            if (conv->output_fn(conv->buf.gbuf.ptr, conv->buf.gbuf.len,
                                conv->output_data)) {
                aushape_conv_buf_empty(&conv->buf);
                conv->events_in_doc = 0;
                conv->in_doc = false;
            } else {
                conv->rc = AUSHAPE_RC_CONV_OUTPUT_FAILED;
            }
        } else {
            conv->rc = rc;
        }
        assert(aushape_conv_is_valid(conv));
    }

    return conv->rc;
}

enum aushape_rc
aushape_conv_input(struct aushape_conv *conv,
                   const char *ptr,
                   size_t len)
{
    if (!aushape_conv_is_valid(conv) ||
        (ptr == NULL && len > 0)) {
        return AUSHAPE_RC_INVALID_ARGS;
    }
    if (conv->events_per_doc == SSIZE_MAX && !conv->in_doc) {
        return AUSHAPE_RC_INVALID_STATE;
    }

    if (conv->rc == AUSHAPE_RC_OK) {
        if (auparse_feed(conv->au, ptr, len) < 0) {
            conv->rc = AUSHAPE_RC_CONV_AUPARSE_FAILED;
        }
    }

    return conv->rc;
}

enum aushape_rc
aushape_conv_flush(struct aushape_conv *conv)
{
    if (!aushape_conv_is_valid(conv)) {
        return AUSHAPE_RC_INVALID_ARGS;
    }
    if (conv->events_per_doc == SSIZE_MAX && !conv->in_doc) {
        return AUSHAPE_RC_INVALID_STATE;
    }

    if (conv->rc == AUSHAPE_RC_OK) {
        if (auparse_flush_feed(conv->au) < 0) {
            conv->rc = AUSHAPE_RC_CONV_AUPARSE_FAILED;
        }
    }

    return conv->rc;
}

void
aushape_conv_destroy(struct aushape_conv *conv)
{
    if (conv != NULL) {
        assert(aushape_conv_is_valid(conv));
        auparse_destroy(conv->au);
        aushape_conv_buf_cleanup(&conv->buf);
        memset(conv, 0, sizeof(*conv));
        free(conv);
    }
}
