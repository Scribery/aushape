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
#include <string.h>

/** Converter */
struct aushape_conv {
    /** Auparse state */
    auparse_state_t            *au;
    /** Output format */
    struct aushape_format       format;
    /** Output function pointer */
    aushape_conv_output_fn      output_fn;
    /** Output function data */
    void                       *output_data;
    /** First conversion failure return code, or OK */
    enum aushape_conv_rc        rc;
    /** Output buffer */
    struct aushape_conv_buf     buf;
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
    enum aushape_conv_rc rc;
    struct aushape_conv *conv = (struct aushape_conv *)data;

    assert(aushape_conv_is_valid(conv));

    if (type != AUPARSE_CB_EVENT_READY) {
        return;
    }
    if (conv->rc != AUSHAPE_CONV_RC_OK) {
        return;
    }

    aushape_conv_buf_empty(&conv->buf);
    rc = aushape_conv_buf_add_event(&conv->buf, &conv->format, 0, au);
    if (rc == AUSHAPE_CONV_RC_OK) {
        if (!conv->output_fn(&conv->format,
                             conv->buf.gbuf.ptr, conv->buf.gbuf.len,
                             conv->output_data)) {
            conv->rc = AUSHAPE_CONV_RC_OUTPUT_FAILED;
        }
    } else {
        conv->rc = rc;
    }
}

enum aushape_conv_rc
aushape_conv_create(struct aushape_conv **pconv,
                    const struct aushape_format *format,
                    aushape_conv_output_fn output_fn,
                    void *output_data)
{
    enum aushape_conv_rc rc;
    struct aushape_conv *conv;

    if (pconv == NULL ||
        !aushape_format_is_valid(format) ||
        output_fn == NULL) {
        rc = AUSHAPE_CONV_RC_INVALID_ARGS;
        goto cleanup;
    }

    conv = calloc(1, sizeof(*conv));
    if (conv == NULL) {
        rc = AUSHAPE_CONV_RC_NOMEM;
        goto cleanup;
    }

    conv->au = auparse_init(AUSOURCE_FEED, NULL);
    if (conv->au == NULL) {
        rc = AUSHAPE_CONV_RC_AUPARSE_FAILED;
        goto cleanup;
    }
    auparse_set_escape_mode(conv->au, AUPARSE_ESC_RAW);
    auparse_add_callback(conv->au, aushape_conv_cb, conv, NULL);

    aushape_conv_buf_init(&conv->buf);
    conv->format = *format;
    conv->output_fn = output_fn;
    conv->output_data = output_data;

    *pconv = conv;
    conv = NULL;
    assert(aushape_conv_is_valid(*pconv));

    rc = AUSHAPE_CONV_RC_OK;

cleanup:
    if (conv != NULL) {
        auparse_destroy(conv->au);
        free(conv);
    }
    return rc;
}

enum aushape_conv_rc
aushape_conv_input(struct aushape_conv *conv,
                   const char *ptr,
                   size_t len)
{
    if (!aushape_conv_is_valid(conv) ||
        (ptr == NULL && len > 0)) {
        return AUSHAPE_CONV_RC_INVALID_ARGS;
    }

    if (conv->rc == AUSHAPE_CONV_RC_OK) {
        if (auparse_feed(conv->au, ptr, len) < 0) {
            conv->rc = AUSHAPE_CONV_RC_AUPARSE_FAILED;
        }
    }

    return conv->rc;
}

enum aushape_conv_rc
aushape_conv_flush(struct aushape_conv *conv)
{
    if (!aushape_conv_is_valid(conv)) {
        return AUSHAPE_CONV_RC_INVALID_ARGS;
    }

    if (conv->rc == AUSHAPE_CONV_RC_OK) {
        if (auparse_flush_feed(conv->au) < 0) {
            conv->rc = AUSHAPE_CONV_RC_AUPARSE_FAILED;
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
