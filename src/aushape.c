/*
 * Copyright (C) 2016 Red Hat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <auparse.h>

/** Output format type */
enum format {
    FORMAT_VOID,    /** Void, invalid format */
    FORMAT_XML,     /** XML */
    FORMAT_JSON,    /** JSON */
    FORMAT_NUM      /** Number of formats (not a format) */
};

/**
 * Check if a format is valid.
 *
 * @param format    The format to check.
 *
 * @return True if the format is valid, false otherwise.
 */
bool
format_is_valid(enum format format)
{
    return format < FORMAT_NUM;
}

/**
 * Check if a format is void.
 *
 * @param format    The format to check.
 *
 * @return True if the format is void, false otherwise.
 */
bool
format_is_void(enum format format)
{
    assert(format_is_valid(format));
    return format == FORMAT_VOID;
}

/** Minimum growing buffer size */
#define GBUF_SIZE_MIN    4096

/** An (exponentially) growing buffer */
struct gbuf {
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
bool
gbuf_is_valid(struct gbuf *gbuf)
{
    return gbuf != NULL &&
           (gbuf->ptr == NULL || gbuf->size > 0) &&
           gbuf->len <= gbuf->size;
}

/**
 * Initialize a growing buffer.
 *
 * @param gbuf  The growing buffer to initialize.
 */
void
gbuf_init(struct gbuf *gbuf)
{
    assert(gbuf != NULL);
    memset(gbuf, 0, sizeof(*gbuf));
    assert(gbuf_is_valid(gbuf));
}

/**
 * Evaluate an expression and return false if it is false.
 *
 * @param _expr The expression to evaluate.
 */
#define GBUF_GUARD(_expr) \
    do {                    \
        if (!(_expr)) {     \
            return false;   \
        }                   \
    } while (0)

/**
 * Cleanup a growing buffer.
 *
 * @param gbuf  The growing buffer to cleanup.
 */
void
gbuf_cleanup(struct gbuf *gbuf)
{
    assert(gbuf_is_valid(gbuf));
    free(gbuf->ptr);
    memset(gbuf, 0, sizeof(*gbuf));
}

/**
 * Empty a growing buffer.
 *
 * @param gbuf  The growing buffer to empty.
 */
void
gbuf_empty(struct gbuf *gbuf)
{
    assert(gbuf_is_valid(gbuf));
    gbuf->len = 0;
}

/**
 * Make sure a growing buffer can accomodate contents of the specified size in
 * bytes.
 *
 * @param gbuf  The growing buffer to accomodate the contents in.
 * @param len   The size of the contents in bytes.
 *
 * @return True if accomodated successfully, false otherwise.
 */
bool
gbuf_accomodate(struct gbuf *gbuf, size_t len)
{
    assert(gbuf_is_valid(gbuf));

    if (len > gbuf->size) {
        char *new_ptr;
        size_t new_size = gbuf->size == 0 ? GBUF_SIZE_MIN : gbuf->size * 2;
        while (new_size < len) {
            new_size *= 2;
        }
        new_ptr = realloc(gbuf->ptr, new_size);
        GBUF_GUARD(new_ptr != NULL);
        gbuf->ptr = new_ptr;
        gbuf->size = new_size;
    }

    return true;
}

/**
 * Add a character to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the character to.
 * @param c     The character to add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
bool
gbuf_add_char(struct gbuf *gbuf, char c)
{
    size_t new_len;
    assert(gbuf_is_valid(gbuf));
    new_len = gbuf->len + 1;
    GBUF_GUARD(gbuf_accomodate(gbuf, new_len));
    gbuf->ptr[gbuf->len] = c;
    gbuf->len = new_len;
    return true;
}

/**
 * Add an abstract buffer to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the abstract buffer to.
 * @param ptr   The pointer to the buffer to add.
 * @param len   The length of the buffer to add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
bool
gbuf_add_buf(struct gbuf *gbuf, const void *ptr, size_t len)
{
    size_t new_len;

    assert(gbuf_is_valid(gbuf));
    assert(ptr != NULL);

    if (len == 0) {
        return true;
    }

    new_len = gbuf->len + len;
    GBUF_GUARD(gbuf_accomodate(gbuf, new_len));

    memcpy(gbuf->ptr + gbuf->len, ptr, len);
    gbuf->len = new_len;

    return true;
}

/**
 * Add an abstract buffer to a growing buffer, lowercasing all characters.
 *
 * @param gbuf  The growing buffer to add the abstract buffer to.
 * @param ptr   The pointer to the buffer to add.
 * @param len   The length of the buffer to add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
bool
gbuf_add_buf_lowercase(struct gbuf *gbuf, const void *ptr, size_t len)
{
    size_t new_len;
    const char *src;
    char *dst;
    char c;

    assert(gbuf_is_valid(gbuf));
    assert(ptr != NULL);

    if (len == 0) {
        return true;
    }

    new_len = gbuf->len + len;
    GBUF_GUARD(gbuf_accomodate(gbuf, new_len));

    for (src = (const char *)ptr, dst = gbuf->ptr + gbuf->len;
         len > 0;
         len--, src++, dst++) {
        c = *src;
        if (c >= 'A' && c <= 'Z') {
            *dst = c + ('a' - 'A');
        } else {
            *dst = c;
        }
    }

    gbuf->len = new_len;

    return true;
}

/**
 * Add a string to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the string to.
 * @param str   The string to add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
bool
gbuf_add_str(struct gbuf *gbuf, const char *str)
{
    assert(gbuf_is_valid(gbuf));
    assert(str != NULL);
    return gbuf_add_buf(gbuf, str, strlen(str));
}

/**
 * Add a string to a growing buffer, lowercasing all characters.
 *
 * @param gbuf  The growing buffer to add the string to.
 * @param str   The string to lowercase and add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
bool
gbuf_add_str_lowercase(struct gbuf *gbuf, const char *str)
{
    assert(gbuf_is_valid(gbuf));
    assert(str != NULL);
    return gbuf_add_buf_lowercase(gbuf, str, strlen(str));
}

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
bool
gbuf_add_fmtv(struct gbuf *gbuf, const char *fmt, va_list ap)
{
    va_list ap_copy;
    int rc;
    size_t len;
    size_t new_len;

    va_copy(ap_copy, ap);
    rc = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);
    GBUF_GUARD(rc >= 0);

    len = (size_t)rc;
    new_len = gbuf->len + len;
    /* NOTE: leaving space for terminating zero */
    GBUF_GUARD(gbuf_accomodate(gbuf, new_len + 1));
    /* NOTE: leaving space for terminating zero */
    rc = vsnprintf(gbuf->ptr + gbuf->len, len + 1, fmt, ap);
    GBUF_GUARD(rc >= 0);
    gbuf->len = new_len;
    return true;
}

/**
 * Add a printf-formatted string to a growing buffer.
 *
 * @param gbuf  The growing buffer to add the formatted string to.
 * @param fmt   The format string.
 * @param ...   The format arguments.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
bool
gbuf_add_fmt(struct gbuf *gbuf, const char *fmt, ...)
{
    va_list ap;
    bool result;

    va_start(ap, fmt);
    result = gbuf_add_fmtv(gbuf, fmt, ap);
    va_end(ap);

    return result;
}

/**
 * Add a string to a growing buffer, escaped as a double-quoted XML attribute
 * value.
 *
 * @param gbuf      The growing buffer to add the escaped string to.
 * @param str       The string to escape and add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
bool
gbuf_add_str_xml(struct gbuf *gbuf, const char *str)
{
    const char *last_p;
    const char *p;
    char c;
    static const char hexdigits[] = "0123456789abcdef";
    char esc_buf[6] = {'&', '#', 'x', 0, 0, ';'};
    const char *esc_ptr;
    size_t esc_len;

    assert(gbuf_is_valid(gbuf));
    assert(str != NULL);

    p = str;
    last_p = p;
    while ((c = *p) != '\0') {
        switch (c) {
#define ESC_CASE(_c, _e) \
        case _c:                    \
            esc_ptr = _e;           \
            esc_len = strlen(_e);   \
            break
        ESC_CASE('"',   "&quot;");
        ESC_CASE('<',   "&lt;");
        ESC_CASE('&',   "&amp;");
#undef ESC_CASE
        default:
            if (c < 0x20 || c == 0x7f) {
                esc_buf[3] = hexdigits[c >> 4];
                esc_buf[4] = hexdigits[c & 0xf];
                esc_ptr = esc_buf;
                esc_len = sizeof(esc_buf);
            } else {
                p++;
                continue;
            }
        }
        GBUF_GUARD(gbuf_add_buf(gbuf, last_p, p - last_p));
        GBUF_GUARD(gbuf_add_buf(gbuf, esc_ptr, esc_len));
        p++;
        last_p = p;
    }
    GBUF_GUARD(gbuf_add_buf(gbuf, last_p, p - last_p));
    return true;
}

/**
 * Add a string to a growing buffer, escaped as a JSON string value.
 *
 * @param gbuf      The growing buffer to add the escaped string to.
 * @param str       The string to escape and add.
 *
 * @return True if added successfully, false if memory allocation failed.
 */
bool
gbuf_add_str_json(struct gbuf *gbuf, const char *str)
{
    const char *last_p;
    const char *p;
    char c;
    static const char hexdigits[] = "0123456789abcdef";
    char esc_char_buf[2] = {'\\'};
    char esc_code_buf[6] = {'\\', 'u', '0', '0'};
    const char *esc_ptr;
    size_t esc_len;

    assert(gbuf_is_valid(gbuf));
    assert(str != NULL);

    p = str;
    last_p = p;
    while ((c = *p) != '\0') {
        switch (c) {
        case '"':
        case '\\':
            esc_char_buf[1] = c;
            esc_ptr = esc_char_buf;
            esc_len = sizeof(esc_char_buf);
            break;
#define ESC_CASE(_c, _e) \
        case _c:                            \
            esc_char_buf[1] = _e;           \
            esc_ptr = esc_char_buf;         \
            esc_len = sizeof(esc_char_buf); \
            break
        ESC_CASE('\b', 'b');
        ESC_CASE('\f', 'f');
        ESC_CASE('\n', 'n');
        ESC_CASE('\r', 'r');
        ESC_CASE('\t', 't');
#undef ESC_CASE
        default:
            if (c < 0x20 || c == 0x7f) {
                esc_code_buf[4] = hexdigits[c >> 4];
                esc_code_buf[5] = hexdigits[c & 0xf];
                esc_ptr = esc_code_buf;
                esc_len = sizeof(esc_code_buf);
            } else {
                p++;
                continue;
            }
            break;
        }
        GBUF_GUARD(gbuf_add_buf(gbuf, last_p, p - last_p));
        GBUF_GUARD(gbuf_add_buf(gbuf, esc_ptr, esc_len));
        p++;
        last_p = p;
    }
    GBUF_GUARD(gbuf_add_buf(gbuf, last_p, p - last_p));
    return true;
}

/**
 * Add a fragment for a field to a growing buffer.
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
gbuf_add_field(struct gbuf *gbuf,
               enum format format,
               bool first,
               const char *name,
               auparse_state_t *au)
{
    const char *value_r;
    const char *value_i = auparse_interpret_field(au);
    int type = auparse_get_field_type(au);

    assert(gbuf_is_valid(gbuf));
    assert(format_is_valid(format));
    assert(!format_is_void(format));
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
    case FORMAT_XML:
        GBUF_GUARD(gbuf_add_str(gbuf, "        <"));
        GBUF_GUARD(gbuf_add_str(gbuf, name));
        GBUF_GUARD(gbuf_add_str(gbuf, " i=\""));
        GBUF_GUARD(gbuf_add_str_xml(gbuf, value_i));
        if (value_r != NULL) {
            GBUF_GUARD(gbuf_add_str(gbuf, "\" r=\""));
            GBUF_GUARD(gbuf_add_str_xml(gbuf, value_r));
        }
        GBUF_GUARD(gbuf_add_str(gbuf, "\"/>\n"));
        break;
    case FORMAT_JSON:
        if (first) {
            GBUF_GUARD(gbuf_add_str(gbuf, "\n                \""));
        } else {
            GBUF_GUARD(gbuf_add_str(gbuf, ",\n                \""));
        }
        GBUF_GUARD(gbuf_add_str(gbuf, name));
        GBUF_GUARD(gbuf_add_str(gbuf, "\": [ \""));
        GBUF_GUARD(gbuf_add_str_json(gbuf, value_i));
        if (value_r != NULL) {
            GBUF_GUARD(gbuf_add_str(gbuf, "\", \""));
            GBUF_GUARD(gbuf_add_str_json(gbuf, value_r));
        }
        GBUF_GUARD(gbuf_add_str(gbuf, "\" ]"));
        break;
    default:
        assert(false);
        return false;
    }

    return true;
}

/**
 * Add a fragment for a record to a growing buffer.
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
gbuf_add_record(struct gbuf *gbuf,
                enum format format,
                bool first,
                auparse_state_t *au)
{
    const char *type_name;
    bool first_field;
    const char *field_name;

    assert(gbuf_is_valid(gbuf));
    assert(format_is_valid(format));
    assert(!format_is_void(format));
    assert(au != NULL);

    type_name = auparse_get_type_name(au);

    if (format == FORMAT_XML) {
        GBUF_GUARD(gbuf_add_str(gbuf, "    <"));
        GBUF_GUARD(gbuf_add_str_lowercase(gbuf, type_name));
        GBUF_GUARD(gbuf_add_str(gbuf, " raw=\""));
        GBUF_GUARD(gbuf_add_str_xml(gbuf, auparse_get_record_text(au)));
        GBUF_GUARD(gbuf_add_str(gbuf, "\">\n"));
    } else {
        if (first) {
            GBUF_GUARD(gbuf_add_str(gbuf, "\n        \""));
        } else {
            GBUF_GUARD(gbuf_add_str(gbuf, ",\n        \""));
        }
        GBUF_GUARD(gbuf_add_str_lowercase(gbuf, type_name));
        GBUF_GUARD(gbuf_add_str(gbuf, "\": {\n"
                                      "            \"raw\" : \""));
        GBUF_GUARD(gbuf_add_str_json(gbuf, auparse_get_record_text(au)));
        GBUF_GUARD(gbuf_add_str(gbuf, "\",\n"
                                      "            \"fields\" : {"));
    }

    auparse_first_field(au);
    first_field = true;
    do {
        field_name = auparse_get_field_name(au);
        if (strcmp(field_name, "type") != 0) {
            GBUF_GUARD(gbuf_add_field(gbuf, format,
                                      first_field, field_name, au));
            first_field = false;
        }
    } while (auparse_next_field(au) > 0);

    if (format == FORMAT_XML) {
        GBUF_GUARD(gbuf_add_str(gbuf, "    </"));
        GBUF_GUARD(gbuf_add_str_lowercase(gbuf, type_name));
        GBUF_GUARD(gbuf_add_str(gbuf, ">\n"));
    } else {
        if (!first_field) {
            GBUF_GUARD(gbuf_add_str(gbuf, "\n            "));
        }
        GBUF_GUARD(gbuf_add_str(gbuf, "}\n"
                                      "        }"));
    }

    return true;
}

/**
 * Add a fragment for an event to a growing buffer.
 *
 * @param gbuf      The growing buffer to add the fragment to.
 * @param format    The output format to use.
 * @param first     True if this is the first event being output,
 *                  false otherwise.
 * @param au        The auparse state with the current event as the one to be
 *                  output.
 *
 * @return True if added successfully, false if memory allocation failed or
 *         an auparse error occurred.
 */
bool
gbuf_add_event(struct gbuf *gbuf,
               enum format format,
               bool first,
               auparse_state_t *au)
{
    const au_event_t *e;
    struct tm *tm;
    char time_buf[32];
    char zone_buf[16];
    char timestamp_buf[64];
    bool first_record;

    assert(gbuf_is_valid(gbuf));
    assert(format_is_valid(format));
    assert(!format_is_void(format));
    assert(au != NULL);

    e = auparse_get_timestamp(au);
    if (e == NULL) {
        fprintf(stderr, "Failed retrieving event data\n");
        return false;
    }

    /* Format timestamp */
    tm = localtime(&e->sec);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", tm);
    strftime(zone_buf, sizeof(zone_buf), "%z", tm);
    snprintf(timestamp_buf, sizeof(timestamp_buf), "%s.%03u%.3s:%s",
             time_buf, e->milli, zone_buf, zone_buf + 3);

    /* Output event header */
    if (format == FORMAT_XML) {
        GBUF_GUARD(gbuf_add_fmt(gbuf, "<event serial=\"%lu\" time=\"%s\"",
                                e->serial, timestamp_buf));
        if (e->host != NULL) {
            GBUF_GUARD(gbuf_add_str(gbuf, " host=\""));
            GBUF_GUARD(gbuf_add_str_xml(gbuf, e->host));
            GBUF_GUARD(gbuf_add_str(gbuf, "\""));
        }
        GBUF_GUARD(gbuf_add_str(gbuf, ">\n"));
    } else {
        if (!first) {
            GBUF_GUARD(gbuf_add_str(gbuf, ",\n"));
        }
        GBUF_GUARD(gbuf_add_fmt(gbuf,
                                "{\n"
                                "    \"serial\" : %lu,\n"
                                "    \"time\" : \"%s\",\n",
                                e->serial, timestamp_buf));
        if (e->host != NULL) {
            GBUF_GUARD(gbuf_add_str(gbuf, "    \"host\" : \""));
            GBUF_GUARD(gbuf_add_str_json(gbuf, e->host));
            GBUF_GUARD(gbuf_add_str(gbuf, "\",\n"));
        }
        GBUF_GUARD(gbuf_add_str(gbuf, "    \"records\" : {"));
    }

    /* Output records */
    if (auparse_first_record(au) <= 0) {
        fprintf(stderr, "Failed rewinding event records\n");
        return false;
    }
    first_record = true;
    do {
        GBUF_GUARD(gbuf_add_record(gbuf, format, first_record, au));
        first_record = false;
    } while(auparse_next_record(au) > 0);

    /* Terminate event */
    if (format == FORMAT_XML) {
        GBUF_GUARD(gbuf_add_str(gbuf, "</event>\n"));
    } else {
        if (first_record) {
            GBUF_GUARD(gbuf_add_str(gbuf,
                                    "}\n"
                                    "}"));
        } else {
            GBUF_GUARD(gbuf_add_str(gbuf,
                                    "\n    }\n"
                                    "}"));
        }
    }

    return true;
}

/** Parsed event callback data */
struct cb_data {
    bool        failed;     /**< True if processing has failed, false if
                                 processing was succesfull so far */
    enum format format;     /**< Output format */
    bool        got_event;  /**< True if output at least one event,
                                 false otherwise */
    struct gbuf gbuf;       /**< Temporary growing buffer for formatting */
};

/**
 * Check if a callback data is valid.
 *
 * @param data  The data to check.
 *
 * @return True if the callback data is valid, false otherwise.
 */
bool
cb_data_is_valid(struct cb_data *data)
{
    return data != NULL &&
           format_is_valid(data->format) &&
           gbuf_is_valid(&data->gbuf);
}

/**
 * Initialize a callback data.
 *
 * @param data      The data to initialize.
 * @param format    The output format to use, cannot be void.
 */
void
cb_data_init(struct cb_data *data, enum format format)
{
    assert(data != NULL);
    assert(format_is_valid(format));
    memset(data, 0, sizeof(*data));
    data->format = format;
    gbuf_init(&data->gbuf);
    assert(cb_data_is_valid(data));
}

/**
 * Cleanup a callback data.
 *
 * @param data      The data to cleanup.
 */
void
cb_data_cleanup(struct cb_data *data)
{
    assert(cb_data_is_valid(data));
    gbuf_cleanup(&data->gbuf);
    memset(data, 0, sizeof(*data));
}

void
cb(auparse_state_t *au, auparse_cb_event_t type, void *opaque)
{
    struct cb_data *data = (struct cb_data *)opaque;

    assert(au != NULL);
    assert(cb_data_is_valid(data));
    assert(!format_is_void(data->format));

    if (type != AUPARSE_CB_EVENT_READY) {
        return;
    }

    if (data->failed) {
        return;
    }

    gbuf_empty(&data->gbuf);
    if (gbuf_add_event(&data->gbuf, data->format, !data->got_event, au)) {
        write(STDOUT_FILENO, data->gbuf.ptr, data->gbuf.len);
        data->got_event = true;
    } else {
        data->failed = true;
    }
}

void
usage(FILE *stream)
{
    fprintf(stream,
            "Usage: aushape FORMAT\n"
            "Convert raw audit log to XML or JSON\n"
            "\n"
            "Arguments:\n"
            "    FORMAT   Output format (\"XML\" or \"JSON\")\n"
            "\n");
}

int
main(int argc, char **argv)
{
    int status = 1;
    enum format format;
    struct cb_data cb_data;
    auparse_state_t *au = NULL;
    char buf[4096];
    const char *str;
    int rc;

    cb_data_init(&cb_data, FORMAT_VOID);

    if (argc != 2) {
        fprintf(stderr, "Output format not specified\n");
        usage(stderr);
        goto cleanup;
    }

    if (strcasecmp(argv[1], "XML") == 0) {
        format = FORMAT_XML;
    } else if (strcasecmp(argv[1], "JSON") == 0) {
        format = FORMAT_JSON;
    } else {
        fprintf(stderr, "Invalid output format: %s\n", argv[1]);
        usage(stderr);
        goto cleanup;
    }
    cb_data.format = format;

    au = auparse_init(AUSOURCE_FEED, NULL);
    if (au == NULL) {
        fprintf(stderr, "Failed creating auparse context\n");
        goto cleanup;
    }
    auparse_set_escape_mode(au, AUPARSE_ESC_RAW);
    auparse_add_callback(au, cb, &cb_data, NULL);

    if (format == FORMAT_XML) {
        str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
              "<log>\n";
    } else {
        str = "[\n";
    }
    write(STDOUT_FILENO, str, strlen(str));
    while (!cb_data.failed &&
           (rc = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        if (auparse_feed(au, buf, sizeof(buf)) < 0) {
            fprintf(stderr, "Failed feeding data to auparse\n");
            goto cleanup;
        }
    }
    if (!cb_data.failed) {
        auparse_flush_feed(au);
    }
    if (format == FORMAT_XML) {
        str = "</log>\n";
    } else {
        str = "\n]\n";
    }
    write(STDOUT_FILENO, str, strlen(str));

    if (rc < 0) {
        fprintf(stderr, "Failed reading stdin: %s\n", strerror(errno));
    }
    status = 0;

cleanup:

    auparse_destroy(au);
    cb_data_cleanup(&cb_data);
    return status;
}
