/*
 * An aushape raw audit log converter output buffer
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

#include <aushape/conv/buf.h>
#include <aushape/disp_coll.h>
#include <aushape/uniq_coll.h>
#include <aushape/execve_coll.h>
#include <aushape/path_coll.h>
#include <aushape/rep_coll.h>
#include <aushape/guard.h>
#include <stdio.h>
#include <string.h>

bool
aushape_conv_buf_is_valid(const struct aushape_conv_buf *buf)
{
    return buf != NULL &&
           aushape_format_is_valid(&buf->format) &&
           aushape_gbuf_is_valid(&buf->gbuf) &&
           aushape_coll_is_valid(buf->coll);
}

enum aushape_rc
aushape_conv_buf_init(struct aushape_conv_buf *buf,
                      const struct aushape_format *format)
{
    static const struct aushape_rep_coll_args obj_pid_args = {
        .name = "obj_pid",
    };
    static const struct aushape_disp_coll_type_link map[] = {
        {
            .name   = "EXECVE",
            .type   = &aushape_execve_coll_type,
            .args   = NULL,
        },
        {
            .name   = "PATH",
            .type   = &aushape_path_coll_type,
            .args   = NULL,
        },
        {
            .name   = "OBJ_PID",
            .type   = &aushape_rep_coll_type,
            .args   = &obj_pid_args,
        },
        {
            .name   = NULL,
            .type   = &aushape_uniq_coll_type,
            .args   = NULL,
        },
    };

    enum aushape_rc rc;

    if (buf == NULL || !aushape_format_is_valid(format)) {
        return AUSHAPE_RC_INVALID_ARGS;
    }
    memset(buf, 0, sizeof(*buf));
    buf->format = *format;
    aushape_gbuf_init(&buf->gbuf);
    aushape_gbuf_init(&buf->data);
    rc = aushape_coll_create(&buf->coll,
                             &aushape_disp_coll_type,
                             &buf->format,
                             /* Divert to separate buffer, if with text */
                             format->with_text ? &buf->data : &buf->gbuf,
                             &map);
    if (rc != AUSHAPE_RC_OK) {
        assert(rc != AUSHAPE_RC_INVALID_ARGS);
        return rc;
    }
    assert(aushape_conv_buf_is_valid(buf));
    return AUSHAPE_RC_OK;
}

void
aushape_conv_buf_cleanup(struct aushape_conv_buf *buf)
{
    assert(aushape_conv_buf_is_valid(buf));
    aushape_gbuf_cleanup(&buf->gbuf);
    aushape_gbuf_cleanup(&buf->data);
    aushape_coll_destroy(buf->coll);
    memset(buf, 0, sizeof(*buf));
}

void
aushape_conv_buf_empty(struct aushape_conv_buf *buf)
{
    assert(aushape_conv_buf_is_valid(buf));
    aushape_gbuf_empty(&buf->gbuf);
    aushape_gbuf_empty(&buf->data);
    aushape_coll_empty(buf->coll);
    assert(aushape_conv_buf_is_valid(buf));
}

enum aushape_rc
aushape_conv_buf_add_event(struct aushape_conv_buf *buf,
                           bool first,
                           auparse_state_t *au)
{
    enum aushape_rc rc;
    size_t level;
    size_t l;
    const au_event_t *e;
    struct tm *tm;
    char time_buf[32];
    char zone_buf[16];
    char timestamp_buf[64];
    bool first_text;
    bool first_data;
    struct aushape_gbuf *gbuf;
    struct aushape_gbuf *data;

    assert(aushape_conv_buf_is_valid(buf));
    assert(au != NULL);

    level = buf->format.events_per_doc != 0;
    l = level;

    /* Divert data to separate buffer if also outputting text */
    data = buf->format.with_text ? &buf->data : &buf->gbuf;
    gbuf = &buf->gbuf;

    e = auparse_get_timestamp(au);
    AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, e != NULL);

    /* Format timestamp */
    tm = localtime(&e->sec);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", tm);
    strftime(zone_buf, sizeof(zone_buf), "%z", tm);
    snprintf(timestamp_buf, sizeof(timestamp_buf), "%s.%03u%.3s:%s",
             time_buf, e->milli, zone_buf, zone_buf + 3);

    /* Output event header */
    if (buf->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_fmt(
                            gbuf, "<event serial=\"%lu\" time=\"%s\"",
                            e->serial, timestamp_buf));
        if (e->host != NULL) {
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, " host=\""));
            AUSHAPE_GUARD(aushape_gbuf_add_str_xml(gbuf, e->host));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\""));
        }
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, ">"));
        l++;
        /* Begin source text, if any */
        if (buf->format.with_text) {
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<text>"));
        }
        /* Begin data */
        AUSHAPE_GUARD(aushape_gbuf_space_opening(data, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(data, "<data>"));
    } else {
        /* Begin event */
        if (!first) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
        }
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf,
                                                 &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '{'));
        l++;
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf,
                                                 &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_fmt(gbuf,
                                           "\"serial\":%lu,", e->serial));
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf,
                                                 &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_fmt(gbuf,
                                           "\"time\":\"%s\",",
                                           timestamp_buf));
        if (e->host != NULL) {
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"host\":\""));
            AUSHAPE_GUARD(aushape_gbuf_add_str_json(gbuf, e->host));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\","));
        }
        /* Begin source text, if any */
        if (buf->format.with_text) {
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"text\":["));
        }
        /* Begin data */
        AUSHAPE_GUARD(aushape_gbuf_space_opening(data, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(data, "\"data\":{"));
    }

    l++;

    /* Output records */
    AUSHAPE_GUARD_BOOL(AUPARSE_FAILED,
                       auparse_first_record(au) >= 0);
    first_text = true;
    first_data = true;
    do {
        /* Add the source text, if requested */
        if (buf->format.with_text) {
            const char *text;
            text = auparse_get_record_text(au);
            AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, text != NULL);
            if (buf->format.lang == AUSHAPE_LANG_XML) {
                AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &buf->format, l));
                AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<line>"));
                AUSHAPE_GUARD(aushape_gbuf_add_str_xml(gbuf, text));
                AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</line>"));
            } else if (buf->format.lang == AUSHAPE_LANG_JSON) {
                if (!first_text) {
                    AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
                }
                AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, &buf->format, l));
                AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '"'));
                AUSHAPE_GUARD(aushape_gbuf_add_str_json(gbuf, text));
                AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '"'));
            }
            first_text = false;
        }
        /* Add the record to the collector */
        rc = aushape_coll_add(buf->coll, l, &first_data, au);
        if (rc != AUSHAPE_RC_OK) {
            assert(rc != AUSHAPE_RC_INVALID_ARGS);
            assert(rc != AUSHAPE_RC_INVALID_STATE);
            assert(aushape_conv_buf_is_valid(buf));
            goto cleanup;
        }
    } while(auparse_next_record(au) > 0);

    /* Make sure the record sequence is complete and added, if any */
    rc = aushape_coll_end(buf->coll, l, &first_data);
    if (rc != AUSHAPE_RC_OK) {
        assert(rc != AUSHAPE_RC_INVALID_ARGS);
        assert(aushape_conv_buf_is_valid(buf));
        goto cleanup;
    }

    l--;

    /* Terminate data */
    if (buf->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(data, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(data, "</data>"));
    } else if (buf->format.lang == AUSHAPE_LANG_JSON) {
        if (!first_data) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(data, &buf->format, l));
        }
        AUSHAPE_GUARD(aushape_gbuf_add_char(data, '}'));
    }

    /* If also outputting the source text */
    if (buf->format.with_text) {
        /* Terminate source text */
        if (buf->format.lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</text>"));
        } else if (buf->format.lang == AUSHAPE_LANG_JSON) {
            if (!first_data) {
                AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &buf->format, l));
            }
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "],"));
        }
        /* Paste the data */
        AUSHAPE_GUARD(aushape_gbuf_add_buf(gbuf, data->ptr, data->len));
    }

    l--;

    /* Terminate event */
    if (buf->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</event>"));
    } else {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '}'));
    }

    assert(l == level);
    rc = AUSHAPE_RC_OK;
cleanup:
    assert(aushape_conv_buf_is_valid(buf));
    return rc;
}

enum aushape_rc
aushape_conv_buf_add_prologue(struct aushape_conv_buf *buf)
{
    enum aushape_rc rc;
    assert(aushape_conv_buf_is_valid(buf));

    AUSHAPE_GUARD(aushape_gbuf_space_opening(&buf->gbuf, &buf->format, 0));
    if (buf->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(
            aushape_gbuf_add_str(
                &buf->gbuf,
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
        /* If level zero is unfolded, which is special for XML */
        if (buf->format.fold_level > 0) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(&buf->gbuf, '\n'));
        }
        AUSHAPE_GUARD(aushape_gbuf_space_opening(&buf->gbuf,
                                                 &buf->format, 0));
        AUSHAPE_GUARD(aushape_gbuf_add_str(&buf->gbuf, "<log>"));
    } else if (buf->format.lang == AUSHAPE_LANG_JSON) {
        AUSHAPE_GUARD(aushape_gbuf_add_char(&buf->gbuf, '['));
    }

    rc = AUSHAPE_RC_OK;
cleanup:
    assert(aushape_conv_buf_is_valid(buf));
    return rc;
}

enum aushape_rc
aushape_conv_buf_add_epilogue(struct aushape_conv_buf *buf)
{
    enum aushape_rc rc;
    assert(aushape_conv_buf_is_valid(buf));

    AUSHAPE_GUARD(aushape_gbuf_space_closing(&buf->gbuf, &buf->format, 0));
    if (buf->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_add_str(&buf->gbuf, "</log>"));
    } else if (buf->format.lang == AUSHAPE_LANG_JSON) {
        AUSHAPE_GUARD(aushape_gbuf_add_char(&buf->gbuf, ']'));
    }

    rc = AUSHAPE_RC_OK;
cleanup:
    assert(aushape_conv_buf_is_valid(buf));
    return rc;
}
