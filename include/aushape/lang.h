/**
 * @brief Aushape output language
 */
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

#ifndef _AUSHAPE_LANG_H
#define _AUSHAPE_LANG_H

#include <stdbool.h>
#include <assert.h>

/** Output language */
enum aushape_lang {
    AUSHAPE_LANG_INVALID, /** Invalid, uninitialized language */
    AUSHAPE_LANG_XML,     /** XML */
    AUSHAPE_LANG_JSON,    /** JSON */
    AUSHAPE_LANG_NUM      /** Number of languages (not a valid language) */
};

/**
 * Check if a language is valid.
 *
 * @param lang      The language to check.
 *
 * @return True if the language is valid, false otherwise.
 */
static inline bool
aushape_lang_is_valid(enum aushape_lang lang)
{
    return lang > AUSHAPE_LANG_INVALID &&
           lang < AUSHAPE_LANG_NUM;
}

#endif /* _AUSHAPE_LANG_H */
