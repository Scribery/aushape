/**
 * A growing buffer tree node.
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

#include <aushape/gbnode.h>
#include <aushape/gbtree.h>
#include <aushape/guard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

bool
aushape_gbnode_is_valid(const struct aushape_gbnode *gbnode)
{
    if (gbnode == NULL) {
        return false;
    }

    if (gbnode->type == AUSHAPE_GBNODE_TYPE_VOID) {
        return true;
    }

    if (!aushape_gbtree_is_valid(gbnode->owner) ||
        gbnode->prev_index >= aushape_garr_get_len(&gbnode->owner->nodes) ||
        gbnode->next_index >= aushape_garr_get_len(&gbnode->owner->nodes)) {
        return false;
    }

    if (gbnode->type == AUSHAPE_GBNODE_TYPE_TEXT) {
        return gbnode->pos + gbnode->len <= gbnode->owner->text.len;
    } else if (gbnode->type == AUSHAPE_GBNODE_TYPE_TREE) {
        return gbnode->tree != NULL;
    }

    return true;
}

bool
aushape_gbnode_is_empty(const struct aushape_gbnode *gbnode)
{
    assert(aushape_gbnode_is_valid(gbnode));
    switch (gbnode->type) {
    case AUSHAPE_GBNODE_TYPE_VOID:
        return true;
    case AUSHAPE_GBNODE_TYPE_TEXT:
        return gbnode->len == 0;
    case AUSHAPE_GBNODE_TYPE_TREE:
        return aushape_gbtree_is_empty(gbnode->tree);
    default:
        return true;
    }
}

bool
aushape_gbnode_is_solid(const struct aushape_gbnode *gbnode)
{
    assert(aushape_gbnode_is_valid(gbnode));
    switch (gbnode->type) {
    case AUSHAPE_GBNODE_TYPE_VOID:
        return false;
    case AUSHAPE_GBNODE_TYPE_TEXT:
        return true;
    case AUSHAPE_GBNODE_TYPE_TREE:
        return aushape_gbtree_is_solid(gbnode->tree);
    default:
        return false;
    }
}

bool
aushape_gbnode_is_atomic(struct aushape_gbnode *gbnode, bool cached)
{
    assert(aushape_gbnode_is_valid(gbnode));

    if (gbnode->type == AUSHAPE_GBNODE_TYPE_VOID) {
        return true;
    } else if (gbnode->type == AUSHAPE_GBNODE_TYPE_TEXT) {
        return true;
    } else if (gbnode->type == AUSHAPE_GBNODE_TYPE_TREE) {
        return aushape_gbtree_is_atomic(gbnode->tree, cached);
    } else {
        return false;
    }
}

size_t
aushape_gbnode_get_len(struct aushape_gbnode *gbnode, bool cached)
{
    assert(aushape_gbnode_is_valid(gbnode));

    if (gbnode->type == AUSHAPE_GBNODE_TYPE_TEXT) {
        return gbnode->len;
    } else if (gbnode->type == AUSHAPE_GBNODE_TYPE_TREE) {
        return aushape_gbtree_get_len(gbnode->tree, cached);
    } else {
        return 0;
    }
}

size_t
aushape_gbnode_trim(struct aushape_gbnode *gbnode,
                    bool atomic_cached,
                    bool len_cached,
                    size_t len)
{
    assert(aushape_gbnode_is_valid(gbnode));

    switch (gbnode->type) {
    case AUSHAPE_GBNODE_TYPE_VOID:
        return 0;
    case AUSHAPE_GBNODE_TYPE_TEXT:
        return gbnode->len;
    case AUSHAPE_GBNODE_TYPE_TREE:
        return aushape_gbtree_trim(gbnode->tree,
                                   atomic_cached, len_cached, len);
    default:
        return 0;
    }
}

enum aushape_rc
aushape_gbnode_render(struct aushape_gbnode *gbnode,
                      struct aushape_gbuf *gbuf)
{
    assert(aushape_gbnode_is_valid(gbnode));
    assert(aushape_gbuf_is_valid(gbuf));

    if (gbnode->type == AUSHAPE_GBNODE_TYPE_TEXT) {
        return aushape_gbuf_add_buf(gbuf,
                                    gbnode->owner->text.ptr + gbnode->pos,
                                    gbnode->len);
    } else if (gbnode->type == AUSHAPE_GBNODE_TYPE_TREE) {
        return aushape_gbtree_render(gbnode->tree, gbuf);
    } else {
        return AUSHAPE_RC_OK;
    }
}

enum aushape_rc
aushape_gbnode_render_dump(const struct aushape_gbnode *gbnode,
                           struct aushape_gbuf *gbuf,
                           const struct aushape_format *format,
                           size_t level,
                           bool first)
{
    enum aushape_rc rc;
    size_t l = level;

    assert(aushape_gbuf_is_valid(gbuf));
    assert(aushape_gbnode_is_valid(gbnode));
    assert(aushape_format_is_valid(format));

    switch (gbnode->type) {
    case AUSHAPE_GBNODE_TYPE_VOID:
        if (format->lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "<void/>"));
        } else if (format->lang == AUSHAPE_LANG_JSON) {
            if (!first) {
                AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
            }
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "{\"type\":\"void\"}"));
        }
        break;
    case AUSHAPE_GBNODE_TYPE_TEXT:
        if (format->lang == AUSHAPE_LANG_XML) {
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_fmt(
                            gbuf,
                            "<text pos=\"%zu\" len=\"%zu\">",
                            gbnode->pos, gbnode->len));
            AUSHAPE_GUARD(aushape_gbuf_add_buf_xml(
                                    gbuf, 
                                    gbnode->owner->text.ptr + gbnode->pos,
                                    gbnode->len));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</text>"));
        } else if (format->lang == AUSHAPE_LANG_JSON) {
            if (!first) {
                AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
            }
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '{'));
            l++;
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"type\":\"text\""));

            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_fmt(gbuf,
                                               "\"pos\":\"%zu\"",
                                               gbnode->pos));

            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_fmt(gbuf,
                                               "\"len\":\"%zu\"",
                                               gbnode->len));

            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
            AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\"buf\":\""));
            AUSHAPE_GUARD(aushape_gbuf_add_buf_json(
                                    gbuf, 
                                    gbnode->owner->text.ptr + gbnode->pos,
                                    gbnode->len));
            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '"'));
            l--;
            AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, format, l));
            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '}'));
        }
        break;
    case AUSHAPE_GBNODE_TYPE_TREE:
        AUSHAPE_GUARD(aushape_gbtree_render_dump(gbnode->tree, gbuf,
                                                 format, l, first));
    default:
        break;
    }

    assert(l == level);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}

enum aushape_rc
aushape_gbnode_print_dump_to_fd(const struct aushape_gbnode *gbnode,
                                int fd,
                                enum aushape_lang lang)
{
    enum aushape_rc rc;
    struct aushape_format format;
    struct aushape_gbuf gbuf;

    assert(fd >= 0);
    assert(aushape_gbnode_is_valid(gbnode));
    assert(aushape_lang_is_valid(lang));

    format = (struct aushape_format){
                    .lang = lang,
                    .fold_level = SIZE_MAX,
                    .nest_indent = 4,
                    .max_event_size = AUSHAPE_FORMAT_MIN_MAX_EVENT_SIZE};

    aushape_gbuf_init(&gbuf, 4096);

    AUSHAPE_GUARD(aushape_gbnode_render_dump(gbnode, &gbuf,
                                             &format, 0, true));
    /* TODO Handle errors */
    write(fd, gbuf.ptr, gbuf.len);

    rc = AUSHAPE_RC_OK;
cleanup:
    aushape_gbuf_cleanup(&gbuf);
    return rc;
}

enum aushape_rc
aushape_gbnode_print_dump_to_file(const struct aushape_gbnode *gbnode,
                                  const char *filename,
                                  enum aushape_lang lang)
{
    int fd;

    assert(filename != NULL);
    assert(aushape_gbnode_is_valid(gbnode));
    assert(aushape_lang_is_valid(lang));

    /* TODO Handle errors */
    fd = open(filename,
              O_CREAT | O_TRUNC | O_WRONLY,
              S_IRUSR | S_IWUSR |
              S_IRGRP | S_IWGRP |
              S_IROTH | S_IWOTH);

    return aushape_gbnode_print_dump_to_fd(gbnode, fd, lang);
}

