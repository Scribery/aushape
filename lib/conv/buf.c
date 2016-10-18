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
#include <stdio.h>
#include <string.h>

/**
 * Evaluate an expression and return false if it is false.
 *
 * @param _expr The expression to evaluate.
 */
#define GUARD_BOOL(_expr) \
    do {                            \
        if (!(_expr)) {             \
            return false;           \
        }                           \
    } while (0)

/**
 * Evaluate an expression and return AUSHAPE_CONV_RC_NOMEM if it is false.
 *
 * @param _expr The expression to evaluate.
 */
#define GUARD_RC(_expr) \
    do {                                    \
        if (!(_expr)) {                     \
            return AUSHAPE_CONV_RC_NOMEM;   \
        }                                   \
    } while (0)

/**
 * Add a formatted fragment for an auparse field to a growing buffer.
 *
 * @param gbuf      The growing buffer to add the fragment to.
 * @param format    The output format to use.
 * @param first     True if this is the first field being output for a record,
 *                  false otherwise.
 * @param name      The field name.
 * @param au        The auparse state with the current field as the one to be
 *                  output.
 *
 * @return True if added succesfully, false if memory allocation failed.
 */
static bool
aushape_conv_buf_add_field(struct aushape_gbuf *gbuf,
                           const struct aushape_format *format,
                           bool first,
                           const char *name,
                           auparse_state_t *au)
{
    const char *value_r;
    const char *value_i = auparse_interpret_field(au);
    int type = auparse_get_field_type(au);

    assert(aushape_gbuf_is_valid(gbuf));
    assert(aushape_format_is_valid(format));
    assert(name != NULL);
    assert(au != NULL);

    switch (type) {
    case AUPARSE_TYPE_ESCAPED:
    case AUPARSE_TYPE_ESCAPED_KEY:
        value_r = NULL;
        break;
    default:
        value_r = auparse_get_field_str(au);
        if (strcmp(value_r, value_i) == 0) {
            value_r = NULL;
        }
        break;
    }

    switch (format->lang) {
    case AUSHAPE_LANG_XML:
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 2));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '<'));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, " i=\""));
        GUARD_BOOL(aushape_gbuf_add_str_xml(gbuf, value_i));
        if (value_r != NULL) {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\" r=\""));
            GUARD_BOOL(aushape_gbuf_add_str_xml(gbuf, value_r));
        }
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\"/>"));
        break;
    case AUSHAPE_LANG_JSON:
        if (!first) {
            GUARD_BOOL(aushape_gbuf_add_char(gbuf, ','));
        }
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 4));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\": ["));
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 5));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
        GUARD_BOOL(aushape_gbuf_add_str_json(gbuf, value_i));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
        if (value_r != NULL) {
            GUARD_BOOL(aushape_gbuf_add_char(gbuf, ','));
            GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 5));
            GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
            GUARD_BOOL(aushape_gbuf_add_str_json(gbuf, value_r));
            GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
        }
        GUARD_BOOL(aushape_gbuf_space_closing(gbuf, format, 4));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, ']'));
        break;
    default:
        assert(false);
        return false;
    }

    return true;
}

/**
 * Add a formatted fragment for an auparse record to a growing buffer.
 *
 * @param gbuf      The growing buffer to add the fragment to.
 * @param format    The output format to use.
 * @param first     True if this is the first record being output,
 *                  false otherwise.
 * @param name      The record (type) name.
 * @param au        The auparse state with the current record as the one to be
 *                  output.
 *
 * @return True if added succesfully, false if memory allocation failed.
 */
static bool
aushape_conv_buf_add_record(struct aushape_gbuf *gbuf,
                            const struct aushape_format *format,
                            bool first,
                            const char *name,
                            auparse_state_t *au)
{
    bool first_field;
    const char *field_name;

    assert(aushape_gbuf_is_valid(gbuf));
    assert(aushape_format_is_valid(format));
    assert(au != NULL);

    if (format->lang == AUSHAPE_LANG_XML) {
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 1));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '<'));
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, " raw=\""));
        /* TODO: Return AUSHAPE_CONV_RC_AUPARSE_FAILED on failure */
        GUARD_BOOL(aushape_gbuf_add_str_xml(
                                        gbuf, auparse_get_record_text(au)));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\">"));
    } else {
        if (!first) {
            GUARD_BOOL(aushape_gbuf_add_char(gbuf, ','));
        }
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 2));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '"'));
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\": {"));
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 3));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\"raw\": \""));
        /* TODO: Return AUSHAPE_CONV_RC_AUPARSE_FAILED on failure */
        GUARD_BOOL(aushape_gbuf_add_str_json(
                                        gbuf, auparse_get_record_text(au)));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\","));
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 3));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\"fields\": {"));
    }

    auparse_first_field(au);
    first_field = true;
    do {
        field_name = auparse_get_field_name(au);
        if (strcmp(field_name, "type") != 0) {
            GUARD_BOOL(aushape_conv_buf_add_field(gbuf, format,
                                      first_field, field_name, au));
            first_field = false;
        }
    } while (auparse_next_field(au) > 0);

    if (format->lang == AUSHAPE_LANG_XML) {
        GUARD_BOOL(aushape_gbuf_space_closing(gbuf, format, 1));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "</"));
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, ">"));
    } else {
        if (!first_field) {
            GUARD_BOOL(aushape_gbuf_space_closing(gbuf, format, 3));
        }
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '}'));
        GUARD_BOOL(aushape_gbuf_space_closing(gbuf, format, 2));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '}'));
    }

    return true;
}

/**
 * Add an aggregated execve record fragment to a growing buffer.
 *
 * @param gbuf      The growing buffer to add the fragment to.
 * @param format    The output format to use.
 * @param first     True if this is the first record being output,
 *                  false otherwise.
 * @param execve    The aggregated execve record to add.
 *
 * @return True if added succesfully, false if memory allocation failed.
 */
static bool
aushape_conv_buf_add_execve(struct aushape_gbuf *gbuf,
                            const struct aushape_format *format,
                            bool first,
                            const struct aushape_conv_execve *execve)
{
    assert(aushape_gbuf_is_valid(gbuf));
    assert(aushape_format_is_valid(format));
    assert(aushape_conv_execve_is_valid(execve));
    assert(aushape_conv_execve_is_complete(execve));

    if (format->lang == AUSHAPE_LANG_XML) {
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 1));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "<execve raw=\""));
        GUARD_BOOL(aushape_gbuf_add_buf_xml(gbuf,
                                            execve->raw.ptr,
                                            execve->raw.len));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\">"));
    } else if (format->lang == AUSHAPE_LANG_JSON) {
        if (!first) {
            GUARD_BOOL(aushape_gbuf_add_char(gbuf, ','));
        }
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 2));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\"execve\": {"));
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 3));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\"raw\": \""));
        GUARD_BOOL(aushape_gbuf_add_buf_json(gbuf,
                                             execve->raw.ptr,
                                             execve->raw.len));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\","));
        GUARD_BOOL(aushape_gbuf_space_opening(gbuf, format, 3));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\"args\": ["));
    }

    GUARD_BOOL(aushape_gbuf_add_buf(gbuf,
                                    execve->args.ptr, execve->args.len));

    if (format->lang == AUSHAPE_LANG_XML) {
        GUARD_BOOL(aushape_gbuf_space_closing(gbuf, format, 1));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "</execve>"));
    } else if (format->lang == AUSHAPE_LANG_JSON) {
        if (execve->args.len > 0) {
            GUARD_BOOL(aushape_gbuf_space_closing(gbuf, format, 3));
        }
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, ']'));
        GUARD_BOOL(aushape_gbuf_space_closing(gbuf, format, 2));
        GUARD_BOOL(aushape_gbuf_add_char(gbuf, '}'));
    }

    return true;
}

bool
aushape_conv_buf_is_valid(const struct aushape_conv_buf *buf)
{
    return buf != NULL &&
           aushape_gbuf_is_valid(&buf->gbuf) &&
           aushape_conv_execve_is_valid(&buf->execve);
}

void
aushape_conv_buf_init(struct aushape_conv_buf *buf)
{
    assert(buf != NULL);
    memset(buf, 0, sizeof(*buf));
    aushape_gbuf_init(&buf->gbuf);
    aushape_conv_execve_init(&buf->execve);
    assert(aushape_conv_buf_is_valid(buf));
}

void
aushape_conv_buf_cleanup(struct aushape_conv_buf *buf)
{
    assert(aushape_conv_buf_is_valid(buf));
    aushape_gbuf_cleanup(&buf->gbuf);
    aushape_conv_execve_cleanup(&buf->execve);
    memset(buf, 0, sizeof(*buf));
}

void
aushape_conv_buf_empty(struct aushape_conv_buf *buf)
{
    assert(aushape_conv_buf_is_valid(buf));
    aushape_gbuf_empty(&buf->gbuf);
    aushape_conv_execve_empty(&buf->execve);
    assert(aushape_conv_buf_is_valid(buf));
}

enum aushape_conv_rc
aushape_conv_buf_add_event(struct aushape_conv_buf *buf,
                           const struct aushape_format *format,
                           auparse_state_t *au)
{
    enum aushape_conv_rc rc;
    const au_event_t *e;
    struct tm *tm;
    char time_buf[32];
    char zone_buf[16];
    char timestamp_buf[64];
    bool first_record;
    const char *record_name;

    assert(aushape_conv_buf_is_valid(buf));
    assert(aushape_format_is_valid(format));
    assert(au != NULL);

    e = auparse_get_timestamp(au);
    if (e == NULL) {
        return AUSHAPE_CONV_RC_AUPARSE_FAILED;
    }

    /* Format timestamp */
    tm = localtime(&e->sec);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", tm);
    strftime(zone_buf, sizeof(zone_buf), "%z", tm);
    snprintf(timestamp_buf, sizeof(timestamp_buf), "%s.%03u%.3s:%s",
             time_buf, e->milli, zone_buf, zone_buf + 3);

    /* Output event header */
    if (format->lang == AUSHAPE_LANG_XML) {
        GUARD_RC(aushape_gbuf_space_opening(&buf->gbuf, format, 0));
        GUARD_RC(aushape_gbuf_add_fmt(
                            &buf->gbuf, "<event serial=\"%lu\" time=\"%s\"",
                            e->serial, timestamp_buf));
        if (e->host != NULL) {
            GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, " host=\""));
            GUARD_RC(aushape_gbuf_add_str_xml(&buf->gbuf, e->host));
            GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "\""));
        }
        GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, ">"));
    } else {
        GUARD_RC(aushape_gbuf_space_opening(&buf->gbuf, format, 0));
        GUARD_RC(aushape_gbuf_add_char(&buf->gbuf, '{'));
        GUARD_RC(aushape_gbuf_space_opening(&buf->gbuf, format, 1));
        GUARD_RC(aushape_gbuf_add_fmt(&buf->gbuf,
                                      "\"serial\": %lu,", e->serial));
        GUARD_RC(aushape_gbuf_space_opening(&buf->gbuf, format, 1));
        GUARD_RC(aushape_gbuf_add_fmt(&buf->gbuf,
                                      "\"time\": \"%s\",", timestamp_buf));
        if (e->host != NULL) {
            GUARD_RC(aushape_gbuf_space_opening(&buf->gbuf, format, 1));
            GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "\"host\": \""));
            GUARD_RC(aushape_gbuf_add_str_json(&buf->gbuf, e->host));
            GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "\","));
        }
        GUARD_RC(aushape_gbuf_space_opening(&buf->gbuf, format, 1));
        GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "\"records\": {"));
    }

    /* Output records */
    if (auparse_first_record(au) <= 0) {
        return AUSHAPE_CONV_RC_AUPARSE_FAILED;
    }
    first_record = true;
    do {
        record_name = auparse_get_type_name(au);
        /* If this is an "execve" record */
        if (strcmp(record_name, "EXECVE") == 0) {
            rc = aushape_conv_execve_add(&buf->execve, format, au);
            if (rc != AUSHAPE_CONV_RC_OK) {
                assert(aushape_conv_buf_is_valid(buf));
                return rc;
            }
            /* If the execve series was finished */
            if (aushape_conv_execve_is_complete(&buf->execve)) {
                GUARD_RC(aushape_conv_buf_add_execve(&buf->gbuf, format,
                                                     first_record,
                                                     &buf->execve));
                first_record = false;
                aushape_conv_execve_empty(&buf->execve);
            }
        } else {
            /* If the execve series wasn't finished */
            if (!aushape_conv_execve_is_complete(&buf->execve)) {
                return AUSHAPE_CONV_RC_INVALID_EXECVE;
            }
            GUARD_RC(aushape_conv_buf_add_record(&buf->gbuf, format,
                                                 first_record, record_name,
                                                 au));
            first_record = false;
        }
    } while(auparse_next_record(au) > 0);

    /* Terminate event */
    if (format->lang == AUSHAPE_LANG_XML) {
        GUARD_RC(aushape_gbuf_space_closing(&buf->gbuf, format, 0));
        GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "</event>"));
    } else {
        if (!first_record) {
            GUARD_RC(aushape_gbuf_space_closing(&buf->gbuf, format, 1));
        }
        GUARD_RC(aushape_gbuf_add_char(&buf->gbuf, '}'));
        GUARD_RC(aushape_gbuf_space_closing(&buf->gbuf, format, 0));
        GUARD_RC(aushape_gbuf_add_char(&buf->gbuf, '}'));
    }

    assert(aushape_conv_buf_is_valid(buf));
    return AUSHAPE_CONV_RC_OK;
}
