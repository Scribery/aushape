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
    RC(INVALID_EXECVE,  "Invalid execve record sequence encountered"),
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
 * @param name      The record (type) name.
 * @param au        The auparse state with the current record as the one to be
 *                  output.
 *
 * @return True if added succesfully, false if memory allocation failed.
 */
bool
aushape_conv_gbuf_add_record(struct aushape_gbuf *gbuf,
                             enum aushape_format format,
                             bool first,
                             const char *name,
                             auparse_state_t *au)
{
    bool first_field;
    const char *field_name;

    assert(aushape_gbuf_is_valid(gbuf));
    assert(aushape_format_is_valid(format));
    assert(au != NULL);

    if (format == AUSHAPE_FORMAT_XML) {
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "    <"));
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, " raw=\""));
        /* TODO: Return AUSHAPE_CONV_RC_AUPARSE_FAILED on failure */
        GUARD_BOOL(aushape_gbuf_add_str_xml(
                                        gbuf, auparse_get_record_text(au)));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\">\n"));
    } else {
        if (first) {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\n        \""));
        } else {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, ",\n        \""));
        }
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, name));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\": {\n"
                                              "            \"raw\" : \""));
        /* TODO: Return AUSHAPE_CONV_RC_AUPARSE_FAILED on failure */
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
        GUARD_BOOL(aushape_gbuf_add_str_lowercase(gbuf, name));
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

/** Execve record aggregation buffer */
struct aushape_conv_execve {
    /** Formatted raw log buffer */
    struct aushape_gbuf raw;
    /** Formatted argument list buffer */
    struct aushape_gbuf args;
    /** Number of arguments expected */
    size_t              arg_num;
    /** Index of the argument being read */
    size_t              arg_idx;
    /** True if argument length is specified */
    bool                got_len;
    /** Index of the argument slice being read */
    size_t              slice_idx;
    /** Total length of the argument being read */
    size_t              len_total;
    /** Length of the argument read so far */
    size_t              len_read;
};

bool
aushape_conv_execve_is_valid(const struct aushape_conv_execve *execve)
{
    return execve != NULL &&
           aushape_gbuf_is_valid(&execve->raw) &&
           aushape_gbuf_is_valid(&execve->args) &&
           execve->arg_idx <= execve->arg_num &&
           (execve->got_len ||
            (execve->slice_idx == 0 && execve->len_total == 0)) &&
           execve->len_read <= execve->len_total;
}

void
aushape_conv_execve_init(struct aushape_conv_execve *execve)
{
    assert(execve != NULL);
    memset(execve, 0, sizeof(*execve));
    aushape_gbuf_init(&execve->raw);
    aushape_gbuf_init(&execve->args);
    assert(aushape_conv_execve_is_valid(execve));
}

void
aushape_conv_execve_cleanup(struct aushape_conv_execve *execve)
{
    assert(aushape_conv_execve_is_valid(execve));
    aushape_gbuf_cleanup(&execve->raw);
    aushape_gbuf_cleanup(&execve->args);
    memset(execve, 0, sizeof(*execve));
}

void
aushape_conv_execve_empty(struct aushape_conv_execve *execve)
{
    assert(aushape_conv_execve_is_valid(execve));
    aushape_gbuf_empty(&execve->raw);
    aushape_gbuf_empty(&execve->args);
    execve->arg_num = 0;
    execve->arg_idx = 0;
    execve->got_len = false;
    execve->len_total = 0;
    execve->len_read = 0;
    assert(aushape_conv_execve_is_valid(execve));
}

/**
 * Aggregate an execve record data into an aggregation buffer.
 *
 * @param execve    The buffer to aggregate the record into.
 * @param format    The output format to use.
 * @param au        The auparse state with the current record as the execve
 *                  record to be aggregated.
 *
 * @return Return code:
 *          AUSHAPE_CONV_RC_OK              - added succesfully,
 *          AUSHAPE_CONV_RC_NOMEM           - memory allocation failed,
 *          AUSHAPE_CONV_RC_AUPARSE_FAILED  - an auparse call failed,
 *          AUSHAPE_CONV_RC_INVALID_EXECVE  - invalid execve record sequence
 *                                            encountered.
 */
enum aushape_conv_rc
aushape_conv_execve_add(struct aushape_conv_execve *execve,
                        enum aushape_format format,
                        auparse_state_t *au)
{
    const char *raw;
    const char *field_name;
    const char *field_value;
    size_t field_len;
    size_t arg_idx;
    size_t slice_idx;
    int end;

    assert(aushape_conv_execve_is_valid(execve));
    assert(aushape_format_is_valid(format));
    assert(au != NULL);

    /*
     * If it's not the first argument being processed,
     * and so it's not the first record
     */
    if (execve->arg_idx > 0) {
        GUARD_RC(aushape_gbuf_add_char(&execve->raw, '\n'));
    }
    raw = auparse_get_record_text(au);
    if (raw == NULL) {
        return AUSHAPE_CONV_RC_AUPARSE_FAILED;
    }
    GUARD_RC(aushape_gbuf_add_str(&execve->raw, raw));

    /*
     * For each field in the record
     */
    auparse_first_field(au);
    do {
        field_name = auparse_get_field_name(au);
        /* If it's the "type" pseudo-field */
        if (strcmp(field_name, "type") == 0) {
            continue;
        /* If it's the number of arguments */
        } else if (strcmp(field_name, "argc") == 0) {
            /* If already processing a series */
            if (execve->arg_num != 0) {
                return AUSHAPE_CONV_RC_INVALID_EXECVE;
            }
            field_value = auparse_get_field_str(au);
            /* If failed to parse the field */
            if (end = 0,
                sscanf(field_value, "%zu%n", &execve->arg_num, &end) < 1 ||
                (size_t)end != strlen(field_value)) {
                return AUSHAPE_CONV_RC_INVALID_EXECVE;
            }
        /* If it's a whole argument */
        } else if (end = 0,
                   sscanf(field_name, "a%zu%n", &arg_idx, &end) >= 1 &&
                   (size_t)end == strlen(field_name)) {
            if (execve->arg_num == 0 ||
                arg_idx != execve->arg_idx ||
                arg_idx >= execve->arg_num) {
                return AUSHAPE_CONV_RC_INVALID_EXECVE;
            }
            if (format == AUSHAPE_FORMAT_XML) {
                GUARD_RC(aushape_gbuf_add_str(&execve->args,
                                              "        <a i=\""));
                GUARD_RC(aushape_gbuf_add_str_xml(
                            &execve->args, auparse_interpret_field(au)));
                GUARD_RC(aushape_gbuf_add_str(&execve->args, "\"/>\n"));
            } else if (format == AUSHAPE_FORMAT_JSON) {
                /* If it's the first argument in the record */
                if (execve->arg_idx == 0) {
                    GUARD_RC(aushape_gbuf_add_str(
                                &execve->args, "\n                \""));
                } else {
                    GUARD_RC(aushape_gbuf_add_str(
                                &execve->args, ",\n                \""));
                }
                GUARD_RC(aushape_gbuf_add_str_json(
                            &execve->args, auparse_interpret_field(au)));
                GUARD_RC(aushape_gbuf_add_char(&execve->args, '"'));
            }
            execve->arg_idx++;
        /* If it's the length of an argument */
        } else if (end = 0,
                   sscanf(field_name, "a%zu_len%n", &arg_idx, &end) >= 1 &&
                   (size_t)end == strlen(field_name)) {
            if (execve->arg_num == 0 ||
                arg_idx != execve->arg_idx ||
                execve->got_len) {
                return AUSHAPE_CONV_RC_INVALID_EXECVE;
            }
            execve->got_len = true;
            field_value = auparse_get_field_str(au);
            /* If failed to parse the field */
            if (end = 0,
                sscanf(field_value, "%zu%n", &execve->len_total, &end) < 1 ||
                (size_t)end != strlen(field_value)) {
                return AUSHAPE_CONV_RC_INVALID_EXECVE;
            }
        /* If it's an argument slice */
        } else if (end = 0,
                   sscanf(field_name, "a%zu[%zu]%n",
                          &arg_idx, &slice_idx, &end) >= 1 &&
                   (size_t)end == strlen(field_name)) {
            if (execve->arg_num == 0 ||
                arg_idx != execve->arg_idx ||
                arg_idx >= execve->arg_num ||
                !execve->got_len ||
                slice_idx != execve->slice_idx) {
                return AUSHAPE_CONV_RC_INVALID_EXECVE;
            }
            field_value = auparse_get_field_str(au);
            field_len = strlen(field_value);
            if (execve->len_read + field_len > execve->len_total) {
                return AUSHAPE_CONV_RC_INVALID_EXECVE;
            }
            /* If we are starting a new argument */
            if (slice_idx == 0) {
                /* Begin argument markup */
                if (format == AUSHAPE_FORMAT_XML) {
                    GUARD_RC(aushape_gbuf_add_str(&execve->args,
                                                  "        <a i=\""));
                } else if (format == AUSHAPE_FORMAT_JSON) {
                    /* If it's the first argument in the record */
                    if (execve->arg_idx == 0) {
                        GUARD_RC(aushape_gbuf_add_str(
                                    &execve->args, "\n                \""));
                    } else {
                        GUARD_RC(aushape_gbuf_add_str(
                                    &execve->args, ",\n                \""));
                    }
                }
            }
            /* Add the slice */
            if (format == AUSHAPE_FORMAT_XML) {
                GUARD_RC(aushape_gbuf_add_str_xml(
                            &execve->args, auparse_interpret_field(au)));
            } else if (format == AUSHAPE_FORMAT_JSON) {
                GUARD_RC(aushape_gbuf_add_str_json(
                            &execve->args, auparse_interpret_field(au)));
            }
            execve->len_read += field_len;
            /* If we have finished the argument */
            if (execve->len_read == execve->len_total) {
                /* End argument markup */
                if (format == AUSHAPE_FORMAT_XML) {
                    GUARD_RC(aushape_gbuf_add_str(&execve->args, "\"/>\n"));
                } else if (format == AUSHAPE_FORMAT_JSON) {
                    GUARD_RC(aushape_gbuf_add_char(&execve->args, '"'));
                }
                execve->got_len = 0;
                execve->slice_idx = 0;
                execve->len_total = 0;
                execve->len_read = 0;
                execve->arg_idx++;
            } else {
                execve->slice_idx++;
            }
        /* If it's something else */
        } else {
            return AUSHAPE_CONV_RC_INVALID_EXECVE;
        }
    } while (auparse_next_field(au) > 0);
    assert(aushape_conv_execve_is_valid(execve));
    return AUSHAPE_CONV_RC_OK;
}

/**
 * Check if an execve aggregation buffer is complete.
 *
 * @param execve    The buffer to check.
 *
 * @return True if the execve aggregation buffer is complete, false otherwise.
 */
bool
aushape_conv_execve_is_complete(const struct aushape_conv_execve *execve)
{
    assert(aushape_conv_execve_is_valid(execve));
    return execve->arg_idx == execve->arg_num;
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
bool
aushape_conv_gbuf_add_execve(struct aushape_gbuf *gbuf,
                             enum aushape_format format,
                             bool first,
                             const struct aushape_conv_execve *execve)
{
    assert(aushape_gbuf_is_valid(gbuf));
    assert(aushape_format_is_valid(format));
    assert(aushape_conv_execve_is_valid(execve));
    assert(aushape_conv_execve_is_complete(execve));

    if (format == AUSHAPE_FORMAT_XML) {
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "    <execve raw=\""));
        GUARD_BOOL(aushape_gbuf_add_buf_xml(gbuf,
                                            execve->raw.ptr,
                                            execve->raw.len));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\">\n"));
    } else if (format == AUSHAPE_FORMAT_JSON) {
        if (first) {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf,
                                            "\n        \"execve\": {\n"
                                            "            \"raw\" : \""));
        } else {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf,
                                            ",\n        \"execve\": {\n"
                                            "            \"raw\" : \""));
        }
        GUARD_BOOL(aushape_gbuf_add_buf_json(gbuf,
                                             execve->raw.ptr,
                                             execve->raw.len));
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\",\n"
                                              "            \"args\" : ["));
    }

    GUARD_BOOL(aushape_gbuf_add_buf(gbuf,
                                    execve->args.ptr, execve->args.len));

    if (format == AUSHAPE_FORMAT_XML) {
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "    </execve>\n"));
    } else if (format == AUSHAPE_FORMAT_JSON) {
        if (execve->args.len > 0) {
            GUARD_BOOL(aushape_gbuf_add_str(gbuf, "\n            "));
        }
        GUARD_BOOL(aushape_gbuf_add_str(gbuf, "]\n"
                                              "        }"));
    }

    return true;
}

/** Converter's output buffer */
struct aushape_conv_buf {
    /** Growing buffer for an output piece */
    struct aushape_gbuf         gbuf;
    /** Execve record aggregation state */
    struct aushape_conv_execve  execve;
};

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

/**
 * Add a formatted fragment for an auparse event to a converter buffer.
 *
 * @param buf       The converter buffer to add the fragment to.
 * @param format    The output format to use.
 * @param first     True if this is the first event being output,
 *                  false otherwise.
 * @param au        The auparse state with the current event as the one to be
 *                  output.
 *
 * @return Return code:
 *          AUSHAPE_CONV_RC_OK              - added succesfully,
 *          AUSHAPE_CONV_RC_NOMEM           - memory allocation failed,
 *          AUSHAPE_CONV_RC_AUPARSE_FAILED  - an auparse call failed,
 *          AUSHAPE_CONV_RC_INVALID_EXECVE  - invalid execve record sequence
 *                                            encountered.
 */
enum aushape_conv_rc
aushape_conv_buf_add_event(struct aushape_conv_buf *buf,
                           enum aushape_format format,
                           bool first,
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
    if (format == AUSHAPE_FORMAT_XML) {
        GUARD_RC(aushape_gbuf_add_fmt(
                            &buf->gbuf, "<event serial=\"%lu\" time=\"%s\"",
                            e->serial, timestamp_buf));
        if (e->host != NULL) {
            GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, " host=\""));
            GUARD_RC(aushape_gbuf_add_str_xml(&buf->gbuf, e->host));
            GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "\""));
        }
        GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, ">\n"));
    } else {
        if (!first) {
            GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, ",\n"));
        }
        GUARD_RC(aushape_gbuf_add_fmt(&buf->gbuf,
                                      "{\n"
                                      "    \"serial\" : %lu,\n"
                                      "    \"time\" : \"%s\",\n",
                                      e->serial, timestamp_buf));
        if (e->host != NULL) {
            GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "    \"host\" : \""));
            GUARD_RC(aushape_gbuf_add_str_json(&buf->gbuf, e->host));
            GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "\",\n"));
        }
        GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "    \"records\" : {"));
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
                GUARD_RC(aushape_conv_gbuf_add_execve(&buf->gbuf, format,
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
            GUARD_RC(aushape_conv_gbuf_add_record(&buf->gbuf, format,
                                                  first_record, record_name,
                                                  au));
            first_record = false;
        }
    } while(auparse_next_record(au) > 0);

    /* Terminate event */
    if (format == AUSHAPE_FORMAT_XML) {
        GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "</event>\n"));
    } else {
        if (first_record) {
            GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "}\n"
                                                      "}"));
        } else {
            GUARD_RC(aushape_gbuf_add_str(&buf->gbuf, "\n    }\n"
                                                      "}"));
        }
    }

    assert(aushape_conv_buf_is_valid(buf));
    return AUSHAPE_CONV_RC_OK;
}

struct aushape_conv {
    /** Auparse state */
    auparse_state_t            *au;
    /** Output format */
    enum aushape_format         format;
    /** Output function pointer */
    aushape_conv_output_fn      output_fn;
    /** Output function data */
    void                       *output_data;
    /** First input/output failure return code, or OK */
    enum aushape_conv_rc        rc;
    /** Output buffer */
    struct aushape_conv_buf     buf;
    /** True if output at least one event already */
    bool                        output_event;
};

bool
aushape_conv_is_valid(const struct aushape_conv *conv)
{
    return conv != NULL &&
           conv->au != NULL &&
           conv->output_fn != NULL &&
           aushape_conv_buf_is_valid(&conv->buf);
}

void
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
    rc = aushape_conv_buf_add_event(&conv->buf, conv->format,
                                    !conv->output_event, au);
    if (rc == AUSHAPE_CONV_RC_OK) {
        if (conv->output_fn(conv->buf.gbuf.ptr, conv->buf.gbuf.len,
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

    aushape_conv_buf_init(&conv->buf);
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
        aushape_conv_buf_cleanup(&conv->buf);
        memset(conv, 0, sizeof(*conv));
        free(conv);
    }
}
