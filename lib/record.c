/**
 * @brief Aushape audit log record manipulation
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

#include <aushape/record.h>
#include <aushape/field.h>
#include <aushape/guard.h>
#include <string.h>

enum aushape_rc
aushape_record_format_fields(struct aushape_gbuf *gbuf,
                             const struct aushape_format *format,
                             size_t level,
                             auparse_state_t *au)
{
    enum aushape_rc rc;
    bool first_field;
    const char *field_name;

    AUSHAPE_GUARD_BOOL(INVALID_ARGS,
                       aushape_gbuf_is_valid(gbuf) &&
                       aushape_format_is_valid(format) &&
                       au != NULL);

    first_field = true;
    if (auparse_first_field(au)) {
        do {
            field_name = auparse_get_field_name(au);
            if (strcmp(field_name, "type") != 0 &&
                strcmp(field_name, "node") != 0) {
                rc = aushape_field_format(gbuf, format, level,
                                          first_field, field_name, au);
                if (rc != AUSHAPE_RC_OK) {
                    assert(rc != AUSHAPE_RC_INVALID_ARGS);
                    goto cleanup;
                }
                first_field = false;
            }
        } while (auparse_next_field(au) > 0);
    }

    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}
enum aushape_rc
aushape_record_format(struct aushape_gbuf *gbuf,
                      const struct aushape_format *format,
                      size_t level,
                      bool first,
                      const char *name,
                      auparse_state_t *au)
{
    enum aushape_rc rc;
    size_t l = level;
    size_t len;

    AUSHAPE_GUARD_BOOL(INVALID_ARGS,
                       aushape_gbuf_is_valid(gbuf) &&
                       aushape_format_is_valid(format) &&
                       name != NULL &&
                       au != NULL);

    if (format->lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '<'));
        AUSHAPE_GUARD(aushape_gbuf_add_str_lowercase(gbuf, name));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '>'));
    } else if (format->lang == AUSHAPE_LANG_JSON) {
        if (!first) {
            AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, ','));
        }
        AUSHAPE_GUARD(aushape_gbuf_space_opening(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '"'));
        AUSHAPE_GUARD(aushape_gbuf_add_str_lowercase(gbuf, name));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "\":"));
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '{'));
    }

    l++;

    len = gbuf->len;
    rc = aushape_record_format_fields(gbuf, format, l, au);
    if (rc != AUSHAPE_RC_OK) {
        assert(rc != AUSHAPE_RC_INVALID_ARGS);
        goto cleanup;
    }

    l--;

    if (format->lang == AUSHAPE_LANG_XML) {
        AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, format, l));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, "</"));
        AUSHAPE_GUARD(aushape_gbuf_add_str_lowercase(gbuf, name));
        AUSHAPE_GUARD(aushape_gbuf_add_str(gbuf, ">"));
    } else if (format->lang == AUSHAPE_LANG_JSON) {
        if (gbuf->len > len) {
            AUSHAPE_GUARD(aushape_gbuf_space_closing(gbuf, format, l));
        }
        AUSHAPE_GUARD(aushape_gbuf_add_char(gbuf, '}'));
    }

    assert(l == level);
    rc = AUSHAPE_RC_OK;
cleanup:
    return rc;
}
