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

#include <aushape/conv_buf.h>
#include <aushape/disp_coll.h>
#include <aushape/drop_coll.h>
#include <aushape/uniq_coll.h>
#include <aushape/execve_coll.h>
#include <aushape/path_coll.h>
#include <aushape/rep_coll.h>
#include <aushape/field.h>
#include <aushape/guard.h>
#include <aushape/misc.h>
#include <stdio.h>
#include <string.h>

bool
aushape_conv_buf_is_valid(const struct aushape_conv_buf *buf)
{
    return buf != NULL &&
           aushape_format_is_valid(&buf->format) &&
           aushape_gbuf_is_valid(&buf->gbuf) &&
           aushape_gbtree_is_valid(&buf->event) &&
           aushape_gbtree_is_valid(&buf->text) &&
           aushape_gbtree_is_valid(&buf->data) &&
           aushape_gbtree_is_valid(&buf->norm) &&
           aushape_coll_is_valid(buf->coll);
}

enum aushape_rc
aushape_conv_buf_init(struct aushape_conv_buf *buf,
                      const struct aushape_format *format)
{
    static const struct aushape_rep_coll_args obj_pid_args = {
        .name = "obj_pid",
    };
    static const struct aushape_rep_coll_args avc_args = {
        .name = "avc",
    };
    static const struct aushape_rep_coll_args netfilter_cfg_args = {
        .name = "netfilter_cfg",
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
            .name   = "AVC",
            .type   = &aushape_rep_coll_type,
            .args   = &avc_args,
        },
        {
            .name   = "NETFILTER_CFG",
            .type   = &aushape_rep_coll_type,
            .args   = &netfilter_cfg_args,
        },
        {
            .name   = "EOE",
            .type   = &aushape_drop_coll_type,
            .args   = NULL,
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
    aushape_gbuf_init(&buf->gbuf, 4096);
    aushape_gbtree_init(&buf->event, 1024, 32, 32);
    aushape_gbtree_init(&buf->text, 4096, 8, 8);
    aushape_gbtree_init(&buf->data, 4096, 256, 256);
    aushape_gbtree_init(&buf->norm, 4096, 32, 32);
    rc = aushape_coll_create(&buf->coll,
                             &aushape_disp_coll_type,
                             &buf->format,
                             &buf->data,
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
    aushape_coll_destroy(buf->coll);
    aushape_gbtree_cleanup(&buf->data);
    aushape_gbtree_cleanup(&buf->text);
    aushape_gbtree_cleanup(&buf->norm);
    aushape_gbtree_cleanup(&buf->event);
    aushape_gbuf_cleanup(&buf->gbuf);
    memset(buf, 0, sizeof(*buf));
}

void
aushape_conv_buf_empty(struct aushape_conv_buf *buf)
{
    assert(aushape_conv_buf_is_valid(buf));
    aushape_gbuf_empty(&buf->gbuf);
    assert(aushape_conv_buf_is_valid(buf));
}

/** Normalized field type */
enum aushape_conv_buf_norm_type {
    /** Metadata field */
    AUSHAPE_CONV_BUF_NORM_TYPE_META,
    /** Positioning field */
    AUSHAPE_CONV_BUF_NORM_TYPE_POS,
    /** Position listing field */
    AUSHAPE_CONV_BUF_NORM_TYPE_POS_LIST,
};

/** Description of a normalized field */
struct aushape_conv_buf_norm_field {
    /** Field name */
    const char                         *name;
    /** Field type */
    enum aushape_conv_buf_norm_type     type;
    /** Field item name for position listing fields */
    const char                         *item_name;
    /** Value extraсtion function for metadata fields */
    const char *(*fn_meta)(auparse_state_t *au);
    /** Cursor position function for positioning fields */
    int (*fn_pos)(auparse_state_t *au);
    /** Iteration beginning function for position listing fields */
    int (*fn_pos_list_first)(auparse_state_t *au);
    /** Iteration continuation function for position listing fields */
    int (*fn_pos_list_next)(auparse_state_t *au);
};

/**
 * Add event normalized data.
 *
 * @param buf   The buffer to add normalized data to.
 * @param level Syntactic nesting level to add normalized data with.
 * @param au    The auparse state with the current event as the event which
 *              normalized data should be added.
 *
 * @return Return code:
 *          AUSHAPE_RC_OK               - added successfully,
 *          AUSHAPE_RC_NOMEM            - memory allocation failed,
 *          AUSHAPE_RC_AUPARSE_FAILED   - an auparse call failed.
 */
static enum aushape_rc
aushape_conv_buf_add_event_norm(struct aushape_conv_buf *buf,
                                size_t level,
                                auparse_state_t *au)
{
    static const struct aushape_conv_buf_norm_field field_list[] = {
#define FIELD_META(_name_tkn, _fn) \
        {.name = #_name_tkn,                        \
         .type = AUSHAPE_CONV_BUF_NORM_TYPE_META,   \
         .fn_meta = _fn}
#define FIELD_POS(_name_tkn, _fn) \
        {.name = #_name_tkn,                        \
         .type = AUSHAPE_CONV_BUF_NORM_TYPE_POS,   \
         .fn_pos = _fn}
#define FIELD_POS_LIST(_name_tkn, _item_name_tkn, _fn_first, _fn_next) \
        {.name = #_name_tkn,                                            \
         .item_name = #_item_name_tkn,                                  \
         .type = AUSHAPE_CONV_BUF_NORM_TYPE_POS_LIST,                   \
         .fn_pos_list_first = _fn_first,                                \
         .fn_pos_list_next = _fn_next}

        FIELD_META(event_kind,          auparse_normalize_get_event_kind),

        FIELD_POS(session,              auparse_normalize_session),

        FIELD_META(subject_kind,        auparse_normalize_subject_kind),
        FIELD_POS(subject_primary,      auparse_normalize_subject_primary),
        FIELD_POS(subject_secondary,    auparse_normalize_subject_secondary),
        FIELD_POS_LIST(subject_attrs, attr,
                                    auparse_normalize_subject_first_attribute,
                                    auparse_normalize_subject_next_attribute),

        FIELD_META(action,              auparse_normalize_get_action),

        FIELD_META(object_kind,         auparse_normalize_object_kind),
        FIELD_POS(object_primary,       auparse_normalize_object_primary),
        FIELD_POS(object_secondary,     auparse_normalize_object_secondary),
        FIELD_POS(object_primary2,      auparse_normalize_object_primary2),
        FIELD_POS_LIST(object_attrs, attr,
                                    auparse_normalize_object_first_attribute,
                                    auparse_normalize_object_next_attribute),

        FIELD_POS(result,               auparse_normalize_get_results),

        FIELD_META(how,                 auparse_normalize_how),

        FIELD_POS(key,                  auparse_normalize_key),

#undef FIELD_POS_LIST
#undef FIELD_POS
#undef FIELD_META
    };

    struct aushape_gbtree *tree = &buf->norm;
    struct aushape_gbuf *gbuf = &tree->text;
    size_t prio;
    enum aushape_rc rc;
    size_t i;
    size_t j;
    int auparse_rc;
    size_t l = level;
    const char *str;

    assert(aushape_conv_buf_is_valid(buf));
    assert(au != NULL);

    AUSHAPE_GUARD_BOOL(AUPARSE_FAILED,
                       auparse_normalize(au, NORM_OPT_ALL) == 0);

    prio = 1;
    for (i = 0; i < AUSHAPE_ARRAY_SIZE(field_list); i++) {
        const struct aushape_conv_buf_norm_field *field = &field_list[i];

        switch (field->type) {
        case AUSHAPE_CONV_BUF_NORM_TYPE_META:
            str = field->fn_meta(au);
            if (str != NULL) {
                AUSHAPE_GUARD(aushape_field_format_props(gbuf, &buf->format,
                                                         l, i == 0, false,
                                                         field->name,
                                                         NULL, str));
                AUSHAPE_GUARD(aushape_gbtree_node_add_text(tree, prio++));
            }
            break;
        case AUSHAPE_CONV_BUF_NORM_TYPE_POS:
            auparse_rc = field->fn_pos(au);
            AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, auparse_rc >= 0);
            if (auparse_rc == 1) {
                AUSHAPE_GUARD(aushape_field_format(gbuf, &buf->format,
                                                   l, i == 0, false,
                                                   field->name, au));
                AUSHAPE_GUARD(aushape_gbtree_node_add_text(tree, prio++));
            }
            break;
        case AUSHAPE_CONV_BUF_NORM_TYPE_POS_LIST:
            auparse_rc = field->fn_pos_list_first(au);
            AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, auparse_rc >= 0);
            if (auparse_rc != 1) {
                break;
            }

            /* Add list prologue */
            switch (buf->format.lang) {
            case AUSHAPE_LANG_XML:
                AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf,
                                                         &buf->format, l));
                AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '<'));
                AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, field->name));
                AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '>'));
                AUSHAPE_GUARD(aushape_gbtree_node_add_text(tree, prio));
                break;
            case AUSHAPE_LANG_JSON:
                if (i > 0) {
                    AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
                }
                AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf,
                                                         &buf->format, l));
                AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '"'));
                AUSHAPE_GUARD(aushape_gbuf_add_str_json(gbuf, field->name));
                AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\":["));
                AUSHAPE_GUARD(aushape_gbtree_node_add_text(tree, prio));
            default:
                break;
            }
            l++;

            /* Add list items */
            j = 0;
            do {
                AUSHAPE_GUARD(aushape_field_format(gbuf, &buf->format, l,
                                                   j == 0, true,
                                                   field->item_name, au));
                AUSHAPE_GUARD(aushape_gbtree_node_add_text(tree, prio + j));
                j++;
                auparse_rc = field->fn_pos_list_next(au);
                AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, auparse_rc >= 0);
            } while (auparse_rc == 1);

            /* Add list epilogue */
            l--;
            switch (buf->format.lang) {
            case AUSHAPE_LANG_XML:
                AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf,
                                                         &buf->format, l));
                AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</"));
                AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, field->name));
                AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '>'));
                AUSHAPE_GUARD(aushape_gbtree_node_add_text(tree, prio));
                break;
            case AUSHAPE_LANG_JSON:
                AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf,
                                                         &buf->format, l));
                AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ']'));
                AUSHAPE_GUARD(aushape_gbtree_node_add_text(tree, prio));
            default:
                break;
            }
            prio += j;
            break;
        default:
            break;
        }
    }

    assert(l == level);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

enum aushape_rc
aushape_conv_buf_add_event(struct aushape_conv_buf *buf,
                           bool first,
                           bool *padded,
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
    size_t line_num;
    size_t record_num;
    struct aushape_gbtree *event_tree = &buf->event;
    struct aushape_gbuf *event_buf = &event_tree->text;
    struct aushape_gbtree *text_tree = &buf->text;
    struct aushape_gbuf *text_buf = &text_tree->text;
    struct aushape_gbtree *data_tree = &buf->data;
    struct aushape_gbuf *data_buf = &data_tree->text;
    struct aushape_gbtree *norm_tree = &buf->norm;
    struct aushape_gbuf *norm_buf = &norm_tree->text;
    const char *line;
    enum aushape_rc error_rc;
    size_t trimmed_node_index;
    size_t error_node_index;
    size_t text_node_index;
    size_t data_node_index;
    size_t len;
    size_t trimmed_len;

    assert(aushape_conv_buf_is_valid(buf));
    assert(padded != NULL);
    assert(au != NULL);
    assert(aushape_coll_is_empty(buf->coll));

    level = buf->format.events_per_doc != 0;
    l = level;

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
        /* Add start tag header node */
        AUSHAPE_GUARD(aushape_gbuf_space_opening(event_buf, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_fmt(
                            event_buf, "<event serial=\"%lu\" time=\"%s\"",
                            e->serial, timestamp_buf));
        if (e->host != NULL) {
            AUSHAPE_GUARD(aushape_gbuf_add_str(event_buf, " node=\""));
            AUSHAPE_GUARD(aushape_gbuf_add_str_xml(event_buf, e->host));
            AUSHAPE_GUARD(aushape_gbuf_add_str(event_buf, "\""));
        }
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(event_tree, 0));
        /* Add empty placeholder node for trimmed attribute */
        trimmed_node_index = aushape_gbtree_get_node_num(event_tree);
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(event_tree, 0));
        /* Add empty placeholder node for error attribute */
        error_node_index = aushape_gbtree_get_node_num(event_tree);
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(event_tree, 0));
        /* Add start tag trailer node */
        AUSHAPE_GUARD(aushape_gbuf_add_str(event_buf, ">"));
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(event_tree, 0));

        l++;

        /* Begin and attach text node */
        AUSHAPE_GUARD(aushape_gbuf_space_opening(text_buf, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(text_buf, "<text>"));
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(text_tree, 0));
        text_node_index = aushape_gbtree_get_node_num(event_tree);
        AUSHAPE_GUARD(aushape_gbtree_node_add_tree(event_tree, 1, text_tree));

        /* Begin and attach data node */
        AUSHAPE_GUARD(aushape_gbuf_space_opening(data_buf, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(data_buf, "<data>"));
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(data_tree, 0));
        data_node_index = aushape_gbtree_get_node_num(event_tree);
        AUSHAPE_GUARD(aushape_gbtree_node_add_tree(event_tree, 2, data_tree));

        /* Begin and attach normalization node, if requested */
        if (buf->format.with_norm) {
            AUSHAPE_GUARD(aushape_gbuf_space_opening(norm_buf,
                                                     &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(norm_buf, "<norm>"));
            AUSHAPE_GUARD(aushape_gbtree_node_add_text(norm_tree, 0));
            AUSHAPE_GUARD(aushape_gbtree_node_add_tree(event_tree, 3,
                                                       norm_tree));
        }
    } else {
        /* Add event header */
        if (!first) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(event_buf, ','));
        }
        AUSHAPE_GUARD(aushape_gbuf_space_opening(event_buf,
                                                 &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(event_buf, '{'));
        l++;

        AUSHAPE_GUARD(aushape_gbuf_space_opening(event_buf,
                                                 &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_fmt(event_buf,
                                           "\"serial\":%lu", e->serial));

        AUSHAPE_GUARD(aushape_gbuf_add_char(event_buf, ','));
        AUSHAPE_GUARD(aushape_gbuf_space_opening(event_buf,
                                                 &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_fmt(event_buf,
                                           "\"time\":\"%s\"",
                                           timestamp_buf));

        if (e->host != NULL) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(event_buf, ','));
            AUSHAPE_GUARD(aushape_gbuf_space_opening(event_buf, &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(event_buf, "\"node\":\""));
            AUSHAPE_GUARD(aushape_gbuf_add_str_json(event_buf, e->host));
            AUSHAPE_GUARD(aushape_gbuf_add_char(event_buf, '"'));
        }
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(event_tree, 0));

        /* Add empty placeholder node for trimmed attribute */
        trimmed_node_index = aushape_gbtree_get_node_num(event_tree);
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(event_tree, 0));
        /* Add empty placeholder node for error attribute */
        error_node_index = aushape_gbtree_get_node_num(event_tree);
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(event_tree, 0));

        /* Begin and attach text node */
        AUSHAPE_GUARD(aushape_gbuf_add_char(text_buf, ','));
        AUSHAPE_GUARD(aushape_gbuf_space_opening(text_buf, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(text_buf, "\"text\":["));
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(text_tree, 0));
        text_node_index = aushape_gbtree_get_node_num(event_tree);
        AUSHAPE_GUARD(aushape_gbtree_node_add_tree(event_tree, 1, text_tree));

        /* Begin and attach data node */
        AUSHAPE_GUARD(aushape_gbuf_add_char(data_buf, ','));
        AUSHAPE_GUARD(aushape_gbuf_space_opening(data_buf, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(data_buf, "\"data\":{"));
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(data_tree, 0));
        data_node_index = aushape_gbtree_get_node_num(event_tree);
        AUSHAPE_GUARD(aushape_gbtree_node_add_tree(event_tree, 2, data_tree));

        /* Begin and attach normalization node, if requested */
        if (buf->format.with_norm) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(norm_buf, ','));
            AUSHAPE_GUARD(aushape_gbuf_space_opening(norm_buf,
                                                     &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(norm_buf, "\"norm\":{"));
            AUSHAPE_GUARD(aushape_gbtree_node_add_text(norm_tree, 0));
            AUSHAPE_GUARD(aushape_gbtree_node_add_tree(event_tree, 3,
                                                       norm_tree));
        }
    }

    l++;

    /* Output raw and parsed records */
    AUSHAPE_GUARD_BOOL(AUPARSE_FAILED,
                       auparse_first_record(au) >= 0);
    line_num = 0;
    record_num = 0;
    error_rc = AUSHAPE_RC_OK;
    do {
        /* Add the source text line */
        line = auparse_get_record_text(au);
        AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, line != NULL);
        if (buf->format.lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_space_opening(text_buf, &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(text_buf, "<line>"));
            AUSHAPE_GUARD(aushape_gbuf_add_str_xml(text_buf, line));
            AUSHAPE_GUARD(aushape_gbuf_add_str(text_buf, "</line>"));
        } else if (buf->format.lang == AUSHAPE_LANG_JSON) {
            if (line_num > 0) {
                AUSHAPE_GUARD(aushape_gbuf_add_char(text_buf, ','));
            }
            AUSHAPE_GUARD(aushape_gbuf_space_opening(text_buf, &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_char(text_buf, '"'));
            AUSHAPE_GUARD(aushape_gbuf_add_str_json(text_buf, line));
            AUSHAPE_GUARD(aushape_gbuf_add_char(text_buf, '"'));
        }
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(text_tree, line_num));
        line_num++;

        if (error_rc == AUSHAPE_RC_OK) {
            /* Add the parsed record to the collector */
            rc = aushape_coll_add(buf->coll, &record_num, l, record_num, au);
            if (rc != AUSHAPE_RC_OK) {
                assert(rc != AUSHAPE_RC_INVALID_ARGS);
                assert(rc != AUSHAPE_RC_INVALID_STATE);
                error_rc = rc;
            }
        }
    } while(auparse_next_record(au) > 0);

    /* Finish the record sequence, if no error has occurred */
    if (error_rc == AUSHAPE_RC_OK) {
        rc = aushape_coll_end(buf->coll, &record_num, l, record_num);
        if (rc != AUSHAPE_RC_OK) {
            assert(rc != AUSHAPE_RC_INVALID_ARGS);
            assert(aushape_conv_buf_is_valid(buf));
            error_rc = rc;
        }
    }

    /* Drop the event if no records were added and no errors occurred */
    if (record_num == 0 && error_rc == AUSHAPE_RC_OK) {
        rc = AUSHAPE_RC_OK;
        goto cleanup;
    }

    /* Add normalized data, if requested */
    if (buf->format.with_norm) {
        AUSHAPE_GUARD(aushape_conv_buf_add_event_norm(buf, l, au));
    }

    l--;

    /* Terminate source text */
    if (buf->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(text_buf, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(text_buf, "</text>"));
    } else if (buf->format.lang == AUSHAPE_LANG_JSON) {
        if (line_num > 0) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(text_buf, &buf->format, l));
        }
        AUSHAPE_GUARD(aushape_gbuf_add_str(text_buf, "]"));
    }
    AUSHAPE_GUARD(aushape_gbtree_node_add_text(text_tree, 0));

    /* Terminate data if no error has occured */
    if (error_rc == AUSHAPE_RC_OK) {
        if (buf->format.lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(data_buf,
                                                     &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(data_buf, "</data>"));
        } else if (buf->format.lang == AUSHAPE_LANG_JSON) {
            if (record_num > 0) {
                AUSHAPE_GUARD(aushape_gbuf_space_closing(data_buf,
                                                         &buf->format, l));
            }
            AUSHAPE_GUARD(aushape_gbuf_add_char(data_buf, '}'));
        }
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(data_tree, 0));
    }

    /* Terminate normalization, if requested */
    if (buf->format.with_norm) {
        if (buf->format.lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(norm_buf,
                                                     &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(norm_buf, "</norm>"));
        } else if (buf->format.lang == AUSHAPE_LANG_JSON) {
            if (line_num > 0) {
                AUSHAPE_GUARD(aushape_gbuf_space_closing(norm_buf,
                                                         &buf->format, l));
            }
            AUSHAPE_GUARD(aushape_gbuf_add_str(norm_buf, "}"));
        }
        AUSHAPE_GUARD(aushape_gbtree_node_add_text(norm_tree, 0));
    }

    /* If an error has occurred */
    if (error_rc != AUSHAPE_RC_OK) {
        /* Remove the possibly invalid data node */
        aushape_gbtree_node_void(event_tree, data_node_index);
        /* Add the error node */
        if (buf->format.lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_add_str(event_buf, " error=\""));
            AUSHAPE_GUARD(aushape_gbuf_add_str_xml(
                                    event_buf, aushape_rc_to_desc(error_rc)));
            AUSHAPE_GUARD(aushape_gbuf_add_char(event_buf, '"'));
        } else if (buf->format.lang == AUSHAPE_LANG_JSON) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(event_buf, ','));
            AUSHAPE_GUARD(aushape_gbuf_space_opening(event_buf,
                                                     &buf->format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(event_buf, "\"error\":\""));
            AUSHAPE_GUARD(aushape_gbuf_add_str_json(
                                    event_buf, aushape_rc_to_desc(error_rc)));
            AUSHAPE_GUARD(aushape_gbuf_add_char(event_buf, '"'));
        }
        AUSHAPE_GUARD(aushape_gbtree_node_put_text(event_tree,
                                                   error_node_index, 0));
    }

    l--;

    /* Terminate event */
    if (buf->format.lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(event_buf, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(event_buf, "</event>"));
    } else {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(event_buf, &buf->format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(event_buf, '}'));
    }
    AUSHAPE_GUARD(aushape_gbtree_node_add_text(event_tree, 0));

    /* If we weren't asked for the source, and no error has occurred */
    if (!buf->format.with_text && error_rc == AUSHAPE_RC_OK) {
        /* Remove the text node */
        aushape_gbtree_node_void(event_tree, text_node_index);
    }

    /*
     * Trim the event, if necessary
     */
    len = aushape_gbtree_get_len(event_tree, false);
    trimmed_len = aushape_gbtree_trim(event_tree, false, true,
                                      buf->format.max_event_size);
    assert(trimmed_len <= buf->format.max_event_size);
    /* If trimmed */
    if (trimmed_len < len) {
        /* Add the trimmed node */
        if (buf->format.lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_add_str(event_buf, " trimmed=\"\""));
        } else if (buf->format.lang == AUSHAPE_LANG_JSON) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(event_buf, ','));
            AUSHAPE_GUARD(aushape_gbuf_space_opening(event_buf,
                                                     &buf->format, level + 1));
            AUSHAPE_GUARD(aushape_gbuf_add_str(event_buf, "\"trimmed\":[]"));
        }
        AUSHAPE_GUARD(aushape_gbtree_node_put_text(event_tree,
                                                   trimmed_node_index, 0));
        /* Trim again */
        len = trimmed_len;
        trimmed_len = aushape_gbtree_trim(event_tree, true, true,
                                          buf->format.max_event_size);
        assert(trimmed_len <= buf->format.max_event_size);
    }

    /* Render the event */
    AUSHAPE_GUARD(aushape_gbtree_render(event_tree, &buf->gbuf));

    assert(l == level);
    *padded = true;
    rc = AUSHAPE_RC_OK;
cleanup:
    aushape_coll_empty(buf->coll);
    aushape_gbtree_empty(event_tree);
    aushape_gbtree_empty(text_tree);
    aushape_gbtree_empty(data_tree);
    aushape_gbtree_empty(norm_tree);
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
