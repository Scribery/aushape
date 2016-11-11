/**
 * Aushape audit log field manipulation
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

#include <aushape/field.h>
#include <aushape/guard.h>
#include <string.h>

enum aushape_rc
aushape_field_format(struct aushape_gbuf *gbuf,
                     const struct aushape_format *format,
                     size_t level,
                     bool first,
                     const char *name,
                     auparse_state_t *au)
{
    enum aushape_rc rc;
    size_t l = level;
    int type;
    const char *value_r;
    const char *value_i;

    if (!aushape_gbuf_is_valid(gbuf) ||
        !aushape_format_is_valid(format) ||
        name == NULL ||
        au == NULL) {
        rc = AUSHAPE_RC_INVALID_ARGS;
        goto cleanup;
    }

    type = auparse_get_field_type(au);
    value_i = auparse_interpret_field(au);
    AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, value_i != NULL);

    switch (type) {
    case AUPARSE_TYPE_ESCAPED:
    case AUPARSE_TYPE_ESCAPED_KEY:
        value_r = NULL;
        break;
    default:
        value_r = auparse_get_field_str(au);
        AUSHAPE_GUARD_BOOL(AUPARSE_FAILED, value_r != NULL);
        if (strcmp(value_r, value_i) == 0) {
            value_r = NULL;
        }
        break;
    }

    switch (format->lang) {
    case AUSHAPE_LANG_XML:
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_char(gbuf, '<'));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_str(gbuf, name));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_str(gbuf, " i=\""));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_str_xml(gbuf, value_i));
        if (value_r != NULL) {
            AUSHAPE_GUARD_BOOL(NOMEM,
                               aushape_gbuf_add_str(gbuf, "\" r=\""));
            AUSHAPE_GUARD_BOOL(NOMEM,
                               aushape_gbuf_add_str_xml(gbuf, value_r));
        }
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_str(gbuf, "\"/>"));
        break;
    case AUSHAPE_LANG_JSON:
        if (!first) {
            AUSHAPE_GUARD_BOOL(NOMEM,
                               aushape_gbuf_add_char(gbuf, ','));
        }
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_char(gbuf, '"'));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_str(gbuf, name));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_str(gbuf, "\":["));
        l++;
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_char(gbuf, '"'));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_str_json(gbuf, value_i));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_char(gbuf, '"'));
        if (value_r != NULL) {
            AUSHAPE_GUARD_BOOL(NOMEM,
                               aushape_gbuf_add_char(gbuf, ','));
            AUSHAPE_GUARD_BOOL(NOMEM,
                               aushape_gbuf_space_opening(gbuf, format, l));
            AUSHAPE_GUARD_BOOL(NOMEM,
                               aushape_gbuf_add_char(gbuf, '"'));
            AUSHAPE_GUARD_BOOL(NOMEM,
                               aushape_gbuf_add_str_json(gbuf, value_r));
            AUSHAPE_GUARD_BOOL(NOMEM,
                               aushape_gbuf_add_char(gbuf, '"'));
        }
        l--;
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_space_closing(gbuf, format, l));
        AUSHAPE_GUARD_BOOL(NOMEM,
                           aushape_gbuf_add_char(gbuf, ']'));
        break;
    default:
        break;
    }

    assert(l == level);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}
