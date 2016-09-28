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
#include <aushape/gbuf.h>
#include <aushape/misc.h>
#include <auparse.h>
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
 * Evaluate an expression and return AUSHAPE_CONV_RC_AUPARSE_FAILED if it is
 * false.
 *
 * @param _expr The expression to evaluate.
 */
#define GUARD_RC(_expr) \
    do {                                            \
        if (!(_expr)) {                             \
            return AUSHAPE_CONV_RC_AUPARSE_FAILED;  \
        }                                           \
    } while (0)

struct aushape_conv_rc_info {
    const char *name;
    const char *desc;
};

static const struct aushape_conv_rc_info \
                        aushape_conv_rc_info_list[AUSHAPE_CONV_RC_NUM] = {
#define RC(_name, _desc) \
    [AUSHAPE_CONV_RC_##_name] = {.name = #_name, .desc = _desc}
    RC(OK,              "Success"),
    RC(INVALID_ARGS,    "Invalid arguments supplied to a function"),
    RC(NOMEM,           "Memory allocation failed"),
    RC(OUTPUT_FAILED,   "Output function failed"),
    RC(AUPARSE_FAILED,  "An underlying auparse call failed"),
#undef RC
};

const char *
aushape_conv_rc_to_name(enum aushape_conv_rc rc)
{
    if ((size_t)rc < AUSHAPE_ARRAY_SIZE(aushape_conv_rc_info_list)) {
        const struct aushape_conv_rc_info *info =
                        aushape_conv_rc_info_list + rc;
        if (info->name != NULL) {
            return info->name;
        }
    }
    return "UNKNOWN";
}

const char *
aushape_conv_rc_to_desc(enum aushape_conv_rc rc)
{
    if ((size_t)rc < AUSHAPE_ARRAY_SIZE(aushape_conv_rc_info_list)) {
        const struct aushape_conv_rc_info *info =
                        aushape_conv_rc_info_list + rc;
        if (info->desc != NULL) {
            return info->desc;
        }
    }
    return "Unknown return code";
}

struct aushape_conv {
    /** Auparse state */
    auparse_state_t        *au;
    /** Output format */
    enum aushape_format     format;
    /** Output function pointer */
    aushape_conv_output_fn  output_fn;
    /** Output function data */
    void                   *output_data;
    /** First input/output failure return code, or OK */
    enum aushape_conv_rc    rc;
    /** Growing buffer for formatting output pieces */
    struct aushape_gbuf     gbuf;
    /** True if output at least one event already */
    bool                    output_event;
};

bool
aushape_conv_is_valid(const struct aushape_conv *conv)
{
    return conv != NULL &&
           conv->au != NULL &&
           conv->output_fn != NULL;
}

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
bool
aushape_conv_gbuf_add_field(struct aushape_gbuf *gbuf,
                            enum aushape_format format,
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

    switch (format) {
    case AUSHAPE_FORMAT_XML:
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "        <"));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, " i=\""));
        GUARD_BOOL(aushape_gbuf_add_str_xml(gbuf, value_i));
        if (value_r != NULL) {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\" r=\""));
            GUARD_BOOL(aushape_gbuf_add_str_xml(gbuf, value_r));
        }
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\"/>\n"));
        break;
    case AUSHAPE_FORMAT_JSON:
        if (first) {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\n                \""));
        } else {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, ",\n                \""));
        }
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\": [ \""));
        GUARD_BOOL(aushape_gbuf_add_str_json(gbuf, value_i));
        if (value_r != NULL) {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\", \""));
            GUARD_BOOL(aushape_gbuf_add_str_json(gbuf, value_r));
        }
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\" ]"));
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
 * @param au        The auparse state with the current record as the one to be
 *                  output.
 *
 * @return True if added succesfully, false if memory allocation failed.
 */
bool
aushape_conv_gbuf_add_record(struct aushape_gbuf *gbuf,
                             enum aushape_format format,
                             bool first,
                             auparse_state_t *au)
{
    const char *type_name;
    bool first_field;
    const char *field_name;

    assert(aushape_gbuf_is_valid(gbuf));
    assert(aushape_format_is_valid(format));
    assert(au != NULL);

    type_name = auparse_get_type_name(au);

    if (format == AUSHAPE_FORMAT_XML) {
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "    <"));
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, type_name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, " raw=\""));
        GUARD_BOOL(aushape_gbuf_add_str_xml(
                                        gbuf, auparse_get_record_text(au)));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\">\n"));
    } else {
        if (first) {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\n        \""));
        } else {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, ",\n        \""));
        }
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, type_name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\": {\n"
                                              "            \"raw\" : \""));
        GUARD_BOOL(aushape_gbuf_add_str_json(
                                        gbuf, auparse_get_record_text(au)));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\",\n"
                                              "            \"fields\" : {"));
    }

    auparse_first_field(au);
    first_field = true;
    do {
        field_name = auparse_get_field_name(au);
        if (strcmp(field_name, "type") != 0) {
            GUARD_BOOL(aushape_conv_gbuf_add_field(gbuf, format,
                                      first_field, field_name, au));
            first_field = false;
        }
    } while (auparse_next_field(au) > 0);

    if (format == AUSHAPE_FORMAT_XML) {
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "    </"));
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, type_name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, ">\n"));
    } else {
        if (!first_field) {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\n            "));
        }
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "}\n"
                                              "        }"));
    }

    return true;
}

/**
 * Add a formatted fragment for an auparse event to a growing buffer.
 *
 * @param gbuf      The growing buffer to add the fragment to.
 * @param format    The output format to use.
 * @param first     True if this is the first event being output,
 *                  false otherwise.
 * @param au        The auparse state with the current event as the one to be
 *                  output.
 *
 * @return Return code:
 *          AUSHAPE_CONV_RC_OK              - added succesfully,
 *          AUSHAPE_CONV_RC_NOMEM           - memory allocation failed,
 *          AUSHAPE_CONV_RC_AUPARSE_FAILED  - an auparse call failed.
 */
enum aushape_conv_rc
aushape_conv_gbuf_add_event(struct aushape_gbuf *gbuf,
                            enum aushape_format format,
                            bool first,
                            auparse_state_t *au)
{
    const au_event_t *e;
    struct tm *tm;
    char time_buf[32];
    char zone_buf[16];
    char timestamp_buf[64];
    bool first_record;

    assert(aushape_gbuf_is_valid(gbuf));
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
    if (format == AUSHAPE_FORMAT_XML) {
        GUARD_RC(aushape_gbuf_add_fmt(
                                gbuf, "<event serial=\"%lu\" time=\"%s\"",
                                e->serial, timestamp_buf));
        if (e->host != NULL) {
            GUARD_RC(aushape_gbuf_add_str(gbuf, " host=\""));
            GUARD_RC(aushape_gbuf_add_str_xml(gbuf, e->host));
            GUARD_RC(aushape_gbuf_add_str(gbuf, "\""));
        }
        GUARD_RC(aushape_gbuf_add_str(gbuf, ">\n"));
    } else {
        if (!first) {
            GUARD_RC(aushape_gbuf_add_str(gbuf, ",\n"));
        }
        GUARD_RC(aushape_gbuf_add_fmt(gbuf, "{\n"
                                            "    \"serial\" : %lu,\n"
                                            "    \"time\" : \"%s\",\n",
                                            e->serial, timestamp_buf));
        if (e->host != NULL) {
            GUARD_RC(aushape_gbuf_add_str(gbuf, "    \"host\" : \""));
            GUARD_RC(aushape_gbuf_add_str_json(gbuf, e->host));
            GUARD_RC(aushape_gbuf_add_str(gbuf, "\",\n"));
        }
        GUARD_RC(aushape_gbuf_add_str(gbuf, "    \"records\" : {"));
    }

    /* Output records */
    if (auparse_first_record(au) <= 0) {
        return AUSHAPE_CONV_RC_AUPARSE_FAILED;
    }
    first_record = true;
    do {
        GUARD_RC(aushape_conv_gbuf_add_record(gbuf, format,
                                              first_record, au));
        first_record = false;
    } while(auparse_next_record(au) > 0);

    /* Terminate event */
    if (format == AUSHAPE_FORMAT_XML) {
        GUARD_RC(aushape_gbuf_add_str(gbuf, "</event>\n"));
    } else {
        if (first_record) {
            GUARD_RC(aushape_gbuf_add_str(gbuf, "}\n"
                                                "}"));
        } else {
            GUARD_RC(aushape_gbuf_add_str(gbuf, "\n    }\n"
                                                "}"));
        }
    }

    return AUSHAPE_CONV_RC_OK;
}

void
aushape_conv_cb(auparse_state_t *au, auparse_cb_event_t type, void *data)
{
    enum aushape_conv_rc rc;
    struct aushape_conv *conv = (struct aushape_conv *)data;

    assert(aushape_conv_is_valid(conv));
    assert(conv->rc == AUSHAPE_CONV_RC_OK);

    if (type != AUPARSE_CB_EVENT_READY) {
        return;
    }

    aushape_gbuf_empty(&conv->gbuf);
    rc = aushape_conv_gbuf_add_event(&conv->gbuf, conv->format,
                                     !conv->output_event, au);
    if (rc == AUSHAPE_CONV_RC_OK) {
        if (conv->output_fn(conv->gbuf.ptr, conv->gbuf.len,
                            conv->output_data)) {
            conv->output_event = true;
        } else {
            conv->rc = AUSHAPE_CONV_RC_OUTPUT_FAILED;
        }
    } else {
        conv->rc = rc;
    }
}

enum aushape_conv_rc
aushape_conv_create(struct aushape_conv **pconv,
                    enum aushape_format format,
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

    aushape_gbuf_init(&conv->gbuf);
    conv->format = format;
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
        aushape_gbuf_cleanup(&conv->gbuf);
        memset(conv, 0, sizeof(*conv));
        free(conv);
    }
}
