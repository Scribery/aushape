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

#include <aushape/gbuf.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * Evaluate an expression and return false if it is false.
 *
 * @param _expr The expression to evaluate.
 */
#define GUARD(_expr) \
    do {                    \
        if (!(_expr)) {     \
            return false;   \
        }                   \
    } while (0)

bool
aushape_gbuf_is_valid(const struct aushape_gbuf *gbuf)
{
    return gbuf != NULL &&
           (gbuf->ptr == NULL || gbuf->size > 0) &&
           gbuf->len <= gbuf->size;
}

void
aushape_gbuf_init(struct aushape_gbuf *gbuf)
{
    assert(gbuf != NULL);
    memset(gbuf, 0, sizeof(*gbuf));
    assert(aushape_gbuf_is_valid(gbuf));
}

void
aushape_gbuf_cleanup(struct aushape_gbuf *gbuf)
{
    assert(aushape_gbuf_is_valid(gbuf));
    free(gbuf->ptr);
    memset(gbuf, 0, sizeof(*gbuf));
}

void
aushape_gbuf_empty(struct aushape_gbuf *gbuf)
{
    assert(aushape_gbuf_is_valid(gbuf));
    gbuf->len = 0;
}

bool
aushape_gbuf_accomodate(struct aushape_gbuf *gbuf, size_t len)
{
    assert(aushape_gbuf_is_valid(gbuf));

    if (len > gbuf->size) {
        char *new_ptr;
        size_t new_size = (gbuf->size == 0
                                ? AUSHAPE_GBUF_SIZE_MIN :
                                gbuf->size * 2);
        while (new_size < len) {
            new_size *= 2;
        }
        new_ptr = realloc(gbuf->ptr, new_size);
        GUARD(new_ptr != NULL);
        gbuf->ptr = new_ptr;
        gbuf->size = new_size;
    }

    return true;
}

bool
aushape_gbuf_add_char(struct aushape_gbuf *gbuf, char c)
{
    size_t new_len;
    assert(aushape_gbuf_is_valid(gbuf));
    new_len = gbuf->len + 1;
    GUARD(aushape_gbuf_accomodate(gbuf, new_len));
    gbuf->ptr[gbuf->len] = c;
    gbuf->len = new_len;
    return true;
}

bool
aushape_gbuf_add_buf(struct aushape_gbuf *gbuf, const void *ptr, size_t len)
{
    size_t new_len;

    assert(aushape_gbuf_is_valid(gbuf));
    assert(ptr != NULL);

    if (len == 0) {
        return true;
    }

    new_len = gbuf->len + len;
    GUARD(aushape_gbuf_accomodate(gbuf, new_len));

    memcpy(gbuf->ptr + gbuf->len, ptr, len);
    gbuf->len = new_len;

    return true;
}

bool
aushape_gbuf_add_buf_lowercase(struct aushape_gbuf *gbuf,
                               const void *ptr, size_t len)
{
    size_t new_len;
    const char *src;
    char *dst;
    char c;

    assert(aushape_gbuf_is_valid(gbuf));
    assert(ptr != NULL);

    if (len == 0) {
        return true;
    }

    new_len = gbuf->len + len;
    GUARD(aushape_gbuf_accomodate(gbuf, new_len));

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

bool
aushape_gbuf_add_str(struct aushape_gbuf *gbuf, const char *str)
{
    assert(aushape_gbuf_is_valid(gbuf));
    assert(str != NULL);
    return aushape_gbuf_add_buf(gbuf, str, strlen(str));
}

bool
aushape_gbuf_add_str_lowercase(struct aushape_gbuf *gbuf, const char *str)
{
    assert(aushape_gbuf_is_valid(gbuf));
    assert(str != NULL);
    return aushape_gbuf_add_buf_lowercase(gbuf, str, strlen(str));
}

bool
aushape_gbuf_add_vfmt(struct aushape_gbuf *gbuf, const char *fmt, va_list ap)
{
    va_list ap_copy;
    int rc;
    size_t len;
    size_t new_len;

    va_copy(ap_copy, ap);
    rc = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);
    GUARD(rc >= 0);

    len = (size_t)rc;
    new_len = gbuf->len + len;
    /* NOTE: leaving space for terminating zero */
    GUARD(aushape_gbuf_accomodate(gbuf, new_len + 1));
    /* NOTE: leaving space for terminating zero */
    rc = vsnprintf(gbuf->ptr + gbuf->len, len + 1, fmt, ap);
    GUARD(rc >= 0);
    gbuf->len = new_len;
    return true;
}

bool
aushape_gbuf_add_fmt(struct aushape_gbuf *gbuf, const char *fmt, ...)
{
    va_list ap;
    bool result;

    va_start(ap, fmt);
    result = aushape_gbuf_add_vfmt(gbuf, fmt, ap);
    va_end(ap);

    return result;
}

bool
aushape_gbuf_add_buf_xml(struct aushape_gbuf *gbuf,
                         const void *ptr, size_t len)
{
    const char *last_p;
    const char *p;
    unsigned char c;
    static const char hexdigits[] = "0123456789abcdef";
    char esc_buf[6] = {'&', '#', 'x', 0, 0, ';'};
    const char *esc_ptr;
    size_t esc_len;

    assert(aushape_gbuf_is_valid(gbuf));
    assert(ptr != NULL || len == 0);

    p = (const char *)ptr;
    last_p = p;
    for (; len > 0; len--) {
        c = *p;
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
        GUARD(aushape_gbuf_add_buf(gbuf, last_p, p - last_p));
        GUARD(aushape_gbuf_add_buf(gbuf, esc_ptr, esc_len));
        p++;
        last_p = p;
    }
    GUARD(aushape_gbuf_add_buf(gbuf, last_p, p - last_p));
    return true;
}

bool
aushape_gbuf_add_str_xml(struct aushape_gbuf *gbuf, const char *str)
{
    assert(aushape_gbuf_is_valid(gbuf));
    assert(str != NULL);
    return aushape_gbuf_add_buf_xml(gbuf, str, strlen(str));
}

bool
aushape_gbuf_add_buf_json(struct aushape_gbuf *gbuf,
                          const void *ptr, size_t len)
{
    const char *last_p;
    const char *p;
    unsigned char c;
    static const char hexdigits[] = "0123456789abcdef";
    char esc_char_buf[2] = {'\\'};
    char esc_code_buf[6] = {'\\', 'u', '0', '0'};
    const char *esc_ptr;
    size_t esc_len;

    assert(aushape_gbuf_is_valid(gbuf));
    assert(ptr != NULL || len == 0);

    p = (const char *)ptr;
    last_p = p;
    for (; len > 0; len--) {
        c = *p;
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
        GUARD(aushape_gbuf_add_buf(gbuf, last_p, p - last_p));
        GUARD(aushape_gbuf_add_buf(gbuf, esc_ptr, esc_len));
        p++;
        last_p = p;
    }
    GUARD(aushape_gbuf_add_buf(gbuf, last_p, p - last_p));
    return true;
}

bool
aushape_gbuf_add_str_json(struct aushape_gbuf *gbuf, const char *str)
{
    assert(aushape_gbuf_is_valid(gbuf));
    assert(str != NULL);
    return aushape_gbuf_add_buf_json(gbuf, str, strlen(str));
}

