/*
 * An execve record aggregation buffer for raw audit log converter
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

#include <aushape/conv/execve.h>
#include <string.h>
#include <stdio.h>

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
            if (arg_idx != execve->arg_idx ||
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
            if (arg_idx != execve->arg_idx ||
                arg_idx >= execve->arg_num ||
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
            if (arg_idx != execve->arg_idx ||
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
