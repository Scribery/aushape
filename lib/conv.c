/*
 * Converter
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

#include <aushape/conv.h>
#include <aushape/conv_buf.h>
#include <aushape/gbuf.h>
#include <aushape/guard.h>
#include <auparse.h>
#include <limits.h>
#include <string.h>

/** Converter */
struct aushape_conv {
    /** Auparse state */
    auparse_state_t            *au;
    /** Output format */
    struct aushape_format       format;
    /** Output */
    struct aushape_output      *output;
    /**
     * True if the output should be destroyed when converter is destroyed.
     * False otherwise.
     */
    bool                        output_owned;
    /** First conversion failure return code, or OK */
    enum aushape_rc             rc;
    /** Output buffer */
    struct aushape_conv_buf     buf;
    /** True if outputting inside of a document, false otherwise */
    bool                        in_doc;
    /**
     * Amount of events in current document.
     * Events, if format.events_per_doc is positive, bytes if negative.
     */
    size_t                      events_in_doc;
};

bool
aushape_conv_is_valid(const struct aushape_conv *conv)
{
    return conv != NULL &&
           conv->au != NULL &&
           aushape_output_is_valid(conv->output) &&
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
        if (conv->format.events_per_doc != 0 &&
            conv->format.events_per_doc != SSIZE_MAX) {
            if (!conv->in_doc) {
                rc = aushape_conv_buf_add_prologue(&conv->buf);
                if (rc == AUSHAPE_RC_OK) {
                    conv->in_doc = true;
                    if (aushape_output_is_cont(conv->output)) {
                        rc = aushape_output_write(conv->output,
                                                  conv->buf.gbuf.ptr,
                                                  conv->buf.gbuf.len);
                        if (rc == AUSHAPE_RC_OK) {
                            aushape_conv_buf_empty(&conv->buf);
                        } else {
                            conv->rc = rc;
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
        bool added = false;
        rc = aushape_conv_buf_add_event(&conv->buf,
                                        conv->events_in_doc == 0, &added, au);
        if (rc == AUSHAPE_RC_OK) {
            if (added) {
                if (conv->format.events_per_doc > 0) {
                    conv->events_in_doc++;
                } else if (conv->format.events_per_doc < 0) {
                    conv->events_in_doc += conv->buf.gbuf.len;
                }
                if (aushape_output_is_cont(conv->output) ||
                    conv->format.events_per_doc == 0) {
                    rc = aushape_output_write(conv->output,
                                              conv->buf.gbuf.ptr,
                                              conv->buf.gbuf.len);
                    if (rc == AUSHAPE_RC_OK) {
                        aushape_conv_buf_empty(&conv->buf);
                    } else {
                        conv->rc = rc;
                    }
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
        if (conv->format.events_per_doc != 0 &&
            conv->format.events_per_doc != SSIZE_MAX) {
            assert(conv->in_doc);
            /* If hit the limit */
            if ((conv->format.events_per_doc > 0 &&
                 conv->events_in_doc >= (size_t)conv->format.events_per_doc) ||
                (conv->format.events_per_doc < 0 &&
                 conv->events_in_doc >= (size_t)-conv->format.events_per_doc)) {
                rc = aushape_conv_buf_add_epilogue(&conv->buf);
                if (rc == AUSHAPE_RC_OK) {
                    rc = aushape_output_write(conv->output,
                                              conv->buf.gbuf.ptr,
                                              conv->buf.gbuf.len);
                    if (rc == AUSHAPE_RC_OK) {
                        aushape_conv_buf_empty(&conv->buf);
                        conv->events_in_doc = 0;
                        conv->in_doc = false;
                    } else {
                        conv->rc = rc;
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
                    const struct aushape_format *format,
                    struct aushape_output *output,
                    bool output_owned)
{
    enum aushape_rc rc;
    struct aushape_conv *conv;

    if (pconv == NULL ||
        !aushape_format_is_valid(format) ||
        !aushape_output_is_valid(output)) {
        rc = AUSHAPE_RC_INVALID_ARGS;
        goto cleanup;
    }

    conv = calloc(1, sizeof(*conv));
    if (conv == NULL) {
        rc = AUSHAPE_RC_NOMEM;
        goto cleanup;
    }

    conv->au = auparse_init(AUSOURCE_FEED, NULL);
    AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, conv->au != NULL);
    auparse_set_escape_mode(conv->au, AUPARSE_ESC_RAW);
    auparse_add_callback(conv->au, aushape_conv_cb, conv, NULL);

    conv->format = *format;
    rc = aushape_conv_buf_init(&conv->buf, &conv->format);
    if (rc != AUSHAPE_RC_OK) {
        assert(rc != AUSHAPE_RC_INVALID_ARGS);
        goto cleanup;
    }
    conv->output = output;
    conv->output_owned = output_owned;

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
    if (conv->format.events_per_doc != SSIZE_MAX) {
        return AUSHAPE_RC_OK;
    }
    if (conv->in_doc) {
        return AUSHAPE_RC_INVALID_STATE;
    }

    if (conv->rc == AUSHAPE_RC_OK) {
        enum aushape_rc rc;
        rc = aushape_conv_buf_add_prologue(&conv->buf);
        if (rc == AUSHAPE_RC_OK) {
            conv->in_doc = true;
            if (aushape_output_is_cont(conv->output)) {
                rc = aushape_output_write(conv->output,
                                          conv->buf.gbuf.ptr,
                                          conv->buf.gbuf.len);
                if (rc == AUSHAPE_RC_OK) {
                    aushape_conv_buf_empty(&conv->buf);
                } else {
                    conv->rc = rc;
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
    if (conv->format.events_per_doc == 0) {
        return AUSHAPE_RC_OK;
    } else if (!conv->in_doc) {
        if (conv->format.events_per_doc == SSIZE_MAX) {
            return AUSHAPE_RC_INVALID_STATE;
        } else {
            return AUSHAPE_RC_OK;
        }
    }

    if (conv->rc == AUSHAPE_RC_OK) {
        enum aushape_rc rc;
        rc = aushape_conv_buf_add_epilogue(&conv->buf);
        if (rc == AUSHAPE_RC_OK) {
            rc = aushape_output_write(conv->output,
                                      conv->buf.gbuf.ptr,
                                      conv->buf.gbuf.len);
            if (rc == AUSHAPE_RC_OK) {
                aushape_conv_buf_empty(&conv->buf);
                conv->events_in_doc = 0;
                conv->in_doc = false;
            } else {
                conv->rc = rc;
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
    if (conv->format.events_per_doc == SSIZE_MAX && !conv->in_doc) {
        return AUSHAPE_RC_INVALID_STATE;
    }

    if (conv->rc == AUSHAPE_RC_OK) {
        if (auparse_feed(conv->au, ptr, len) < 0) {
            conv->rc = AUSHAPE_RC_AUPARSE_FAILED;
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
    if (conv->format.events_per_doc == SSIZE_MAX && !conv->in_doc) {
        return AUSHAPE_RC_INVALID_STATE;
    }

    if (conv->rc == AUSHAPE_RC_OK) {
        if (auparse_flush_feed(conv->au) < 0) {
            conv->rc = AUSHAPE_RC_AUPARSE_FAILED;
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
        if (conv->output_owned) {
            aushape_output_destroy(conv->output);
        }
        memset(conv, 0, sizeof(*conv));
        free(conv);
    }
}
