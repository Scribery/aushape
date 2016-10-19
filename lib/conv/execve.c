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
 * Exit with the specified return code if a bool expression is false.
 * To exit stores return code in "rc" variable and goes to "cleanup" label.
 *
 * @param _RC_TOKEN     The return code name without the AUSHAPE_CONV_RC_
 *                      prefix.
 * @param _bool_expr    The bool expression to evaluate.
 */
#define GUARD_BOOL(_RC_TOKEN, _bool_expr) \
    do {                                        \
        if (!(_bool_expr)) {                    \
            rc = AUSHAPE_CONV_RC_##_RC_TOKEN;   \
            goto cleanup;                       \
        }                                       \
    } while (0)

/**
 * If an expression producing return code doesn't evaluate to
 * AUSHAPE_CONV_RC_OK, then exit with the evaluated return code. Stores the
 * evaluated return code in "rc" variable, goes to "cleanup" label to exit.
 *
 * @param _rc_expr  The return code expression to evaluate.
 */
#define GUARD(_rc_expr) \
    do {                                \
        rc = (_rc_expr);                \
        if (rc != AUSHAPE_CONV_RC_OK) { \
            goto cleanup;               \
        }                               \
    } while (0)

/**
 * Process "argc" field for the execve record being aggregated.
 *
 * @param execve    The aggregation buffer to process the field for.
 * @param au        The auparse context with the field to be processed as the
 *                  current field.
 *
 * @return Return code:
 *          AUSHAPE_CONV_RC_OK              - added succesfully,
 *          AUSHAPE_CONV_RC_AUPARSE_FAILED  - an auparse call failed,
 *          AUSHAPE_CONV_RC_INVALID_EXECVE  - invalid execve record sequence
 *                                            encountered.
 */
static enum aushape_conv_rc
aushape_conv_execve_add_argc(struct aushape_conv_execve *execve,
                             auparse_state_t *au)
{
    enum aushape_conv_rc rc;
    const char *str;
    size_t num;
    int end;

    assert(aushape_conv_execve_is_valid(execve));
    assert(au != NULL);

    GUARD_BOOL(INVALID_EXECVE, execve->arg_num == 0);
    str = auparse_get_field_str(au);
    GUARD_BOOL(AUPARSE_FAILED, str != NULL);
    end = 0;
    GUARD_BOOL(INVALID_EXECVE,
               sscanf(str, "%zu%n", &num, &end) >= 1 &&
               (size_t)end == strlen(str));
    execve->arg_num = num;
    rc = AUSHAPE_CONV_RC_OK;
cleanup:
    assert(aushape_conv_execve_is_valid(execve));
    return rc;
}

/**
 * Process "a[0-9]+" field for the execve record being aggregated.
 *
 * @param execve    The aggregation buffer to process the field for.
 * @param format    The output format to use.
 * @param arg_idx   The argument index from the field name.
 * @param au        The auparse context with the field to be processed as the
 *                  current field.
 *
 * @return Return code:
 *          AUSHAPE_CONV_RC_OK              - added succesfully,
 *          AUSHAPE_CONV_RC_NOMEM           - memory allocation failed,
 *          AUSHAPE_CONV_RC_AUPARSE_FAILED  - an auparse call failed,
 *          AUSHAPE_CONV_RC_INVALID_EXECVE  - invalid execve record sequence
 *                                            encountered.
 */
static enum aushape_conv_rc
aushape_conv_execve_add_arg(struct aushape_conv_execve *execve,
                            const struct aushape_format *format,
                            size_t arg_idx,
                            auparse_state_t *au)
{
    enum aushape_conv_rc rc;
    const char *str;

    assert(aushape_conv_execve_is_valid(execve));
    assert(aushape_format_is_valid(format));
    assert(au != NULL);

    GUARD_BOOL(INVALID_EXECVE,
               arg_idx == execve->arg_idx &&
               arg_idx < execve->arg_num);

    str = auparse_interpret_field(au);
    GUARD_BOOL(AUPARSE_FAILED, str != NULL);
    if (format->lang == AUSHAPE_LANG_XML) {
        GUARD_BOOL(NOMEM, aushape_gbuf_space_opening(&execve->args, format, 2));
        GUARD_BOOL(NOMEM, aushape_gbuf_add_str(&execve->args, "<a i=\""));
        GUARD_BOOL(NOMEM, aushape_gbuf_add_str_xml(&execve->args, str));
        GUARD_BOOL(NOMEM, aushape_gbuf_add_str(&execve->args, "\"/>"));
    } else if (format->lang == AUSHAPE_LANG_JSON) {
        /* If it's not the first argument in the record */
        if (execve->arg_idx > 0) {
            GUARD_BOOL(NOMEM, aushape_gbuf_add_char(&execve->args, ','));
        }
        GUARD_BOOL(NOMEM, aushape_gbuf_space_opening(&execve->args, format, 4));
        GUARD_BOOL(NOMEM, aushape_gbuf_add_char(&execve->args, '"'));
        GUARD_BOOL(NOMEM, aushape_gbuf_add_str_json(&execve->args, str));
        GUARD_BOOL(NOMEM, aushape_gbuf_add_char(&execve->args, '"'));
    }

    execve->arg_idx++;

    rc = AUSHAPE_CONV_RC_OK;
cleanup:
    assert(aushape_conv_execve_is_valid(execve));
    return rc;
}

/**
 * Process "a[0-9]+_len" field for the execve record being aggregated.
 *
 * @param execve    The aggregation buffer to process the field for.
 * @param arg_idx   The argument index from the field name.
 * @param au        The auparse context with the field to be processed as the
 *                  current field.
 *
 * @return Return code:
 *          AUSHAPE_CONV_RC_OK              - added succesfully,
 *          AUSHAPE_CONV_RC_AUPARSE_FAILED  - an auparse call failed,
 *          AUSHAPE_CONV_RC_INVALID_EXECVE  - invalid execve record sequence
 *                                            encountered.
 */
static enum aushape_conv_rc
aushape_conv_execve_add_arg_len(struct aushape_conv_execve *execve,
                                size_t arg_idx,
                                auparse_state_t *au)
{
    enum aushape_conv_rc rc;
    const char *str;
    size_t num;
    int end;

    assert(aushape_conv_execve_is_valid(execve));
    assert(au != NULL);

    GUARD_BOOL(INVALID_EXECVE,
               arg_idx == execve->arg_idx &&
               arg_idx < execve->arg_num &&
               !execve->got_len);

    execve->got_len = true;

    str = auparse_get_field_str(au);
    GUARD_BOOL(AUPARSE_FAILED, str != NULL);
    end = 0;
    GUARD_BOOL(INVALID_EXECVE,
               sscanf(str, "%zu%n", &num, &end) >= 1 &&
               (size_t)end == strlen(str));
    execve->len_total = num;

    rc = AUSHAPE_CONV_RC_OK;
cleanup:
    assert(aushape_conv_execve_is_valid(execve));
    return rc;
}

/**
 * Process "a[0-9]\[[0-9]+\]" field for the execve record being aggregated.
 *
 * @param execve    The aggregation buffer to process the field for.
 * @param format    The output format to use.
 * @param arg_idx   The argument index from the field name.
 * @param slice_idx The slice index from the field name.
 * @param au        The auparse context with the field to be processed as the
 *                  current field.
 *
 * @return Return code:
 *          AUSHAPE_CONV_RC_OK              - added succesfully,
 *          AUSHAPE_CONV_RC_NOMEM           - memory allocation failed,
 *          AUSHAPE_CONV_RC_AUPARSE_FAILED  - an auparse call failed,
 *          AUSHAPE_CONV_RC_INVALID_EXECVE  - invalid execve record sequence
 *                                            encountered.
 */
static enum aushape_conv_rc
aushape_conv_execve_add_arg_slice(struct aushape_conv_execve *execve,
                                  const struct aushape_format *format,
                                  size_t arg_idx, size_t slice_idx,
                                  auparse_state_t *au)
{
    enum aushape_conv_rc rc;
    const char *str;
    size_t len;

    assert(aushape_conv_execve_is_valid(execve));
    assert(aushape_format_is_valid(format));
    assert(au != NULL);

    GUARD_BOOL(INVALID_EXECVE,
               arg_idx == execve->arg_idx &&
               arg_idx < execve->arg_num &&
               execve->got_len &&
               slice_idx == execve->slice_idx);

    str = auparse_get_field_str(au);
    GUARD_BOOL(AUPARSE_FAILED, str != NULL);
    len = strlen(str);
    GUARD_BOOL(INVALID_EXECVE,
               execve->len_read + len <= execve->len_total);
    /* If we are starting a new argument */
    if (slice_idx == 0) {
        /* Begin argument markup */
        if (format->lang == AUSHAPE_LANG_XML) {
            GUARD_BOOL(NOMEM, aushape_gbuf_space_opening(&execve->args,
                                                         format, 2));
            GUARD_BOOL(NOMEM, aushape_gbuf_add_str(&execve->args, "<a i=\""));
        } else if (format->lang == AUSHAPE_LANG_JSON) {
            /* If it's not the first argument in the record */
            if (execve->arg_idx > 0) {
                GUARD_BOOL(NOMEM, aushape_gbuf_add_char(&execve->args, ','));
            }
            GUARD_BOOL(NOMEM, aushape_gbuf_space_opening(&execve->args,
                                                         format, 4));
            GUARD_BOOL(NOMEM, aushape_gbuf_add_char(&execve->args, '"'));
        }
    }
    /* Add the slice */
    str = auparse_interpret_field(au);
    GUARD_BOOL(AUPARSE_FAILED, str != NULL);
    if (format->lang == AUSHAPE_LANG_XML) {
        GUARD_BOOL(NOMEM, aushape_gbuf_add_str_xml(&execve->args, str));
    } else if (format->lang == AUSHAPE_LANG_JSON) {
        GUARD_BOOL(NOMEM, aushape_gbuf_add_str_json(&execve->args, str));
    }
    execve->len_read += len;
    /* If we have finished the argument */
    if (execve->len_read == execve->len_total) {
        /* End argument markup */
        if (format->lang == AUSHAPE_LANG_XML) {
            GUARD_BOOL(NOMEM, aushape_gbuf_add_str(&execve->args, "\"/>"));
        } else if (format->lang == AUSHAPE_LANG_JSON) {
            GUARD_BOOL(NOMEM, aushape_gbuf_add_char(&execve->args, '"'));
        }
        execve->got_len = 0;
        execve->slice_idx = 0;
        execve->len_total = 0;
        execve->len_read = 0;
        execve->arg_idx++;
    } else {
        execve->slice_idx++;
    }

    rc = AUSHAPE_CONV_RC_OK;
cleanup:
    assert(aushape_conv_execve_is_valid(execve));
    return rc;
}

enum aushape_conv_rc
aushape_conv_execve_add(struct aushape_conv_execve *execve,
                        const struct aushape_format *format,
                        auparse_state_t *au)
{
    enum aushape_conv_rc rc;
    const char *raw;
    const char *field_name;
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
        GUARD_BOOL(NOMEM, aushape_gbuf_add_char(&execve->raw, '\n'));
    }
    raw = auparse_get_record_text(au);
    GUARD_BOOL(AUPARSE_FAILED, raw != NULL);
    GUARD_BOOL(NOMEM, aushape_gbuf_add_str(&execve->raw, raw));

    /*
     * For each field in the record
     */
    GUARD_BOOL(INVALID_EXECVE, auparse_first_field(au) != 0);
    do {
        field_name = auparse_get_field_name(au);
        GUARD_BOOL(AUPARSE_FAILED, field_name != NULL);
        /* If it's the "type" pseudo-field */
        if (strcmp(field_name, "type") == 0) {
            continue;
        /* If it's the number of arguments */
        } else if (strcmp(field_name, "argc") == 0) {
            GUARD(aushape_conv_execve_add_argc(execve, au));
        /* If it's a whole argument */
        } else if (end = 0,
                   sscanf(field_name, "a%zu%n", &arg_idx, &end) >= 1 &&
                   (size_t)end == strlen(field_name)) {
            GUARD(aushape_conv_execve_add_arg(execve, format, arg_idx, au));
        /* If it's the length of an argument */
        } else if (end = 0,
                   sscanf(field_name, "a%zu_len%n", &arg_idx, &end) >= 1 &&
                   (size_t)end == strlen(field_name)) {
            GUARD(aushape_conv_execve_add_arg_len(execve, arg_idx, au));
        /* If it's an argument slice */
        } else if (end = 0,
                   sscanf(field_name, "a%zu[%zu]%n",
                          &arg_idx, &slice_idx, &end) >= 2 &&
                   (size_t)end == strlen(field_name)) {
            GUARD(aushape_conv_execve_add_arg_slice(execve, format,
                                                    arg_idx, slice_idx, au));
        /* If it's something else */
        } else {
            rc = AUSHAPE_CONV_RC_INVALID_EXECVE;
            goto cleanup;
        }
    } while (auparse_next_field(au) > 0);

    rc = AUSHAPE_CONV_RC_OK;
cleanup:
    assert(aushape_conv_execve_is_valid(execve));
    return rc;
}
