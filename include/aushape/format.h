/**
 * @brief Aushape output format.
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

#ifndef _AUSHAPE_FORMAT_H
#define _AUSHAPE_FORMAT_H

#include <aushape/lang.h>
#include <stdbool.h>
#include <stddef.h>

/** Output format */
struct aushape_format {
    /** Output language */
    enum aushape_lang   lang;
    /**
     * Syntactic nesting level at which the output should be
     * "folded" into single line. Zero for the whole output to be on the same
     * line. SIZE_MAX for the output to be fully "unfolded". Total number of
     * nesting levels depends on output language.
     */
    size_t              fold_level;
    /** Initial indentation of each output line, in spaces */
    size_t              init_indent;
    /** Indentation for each nesting level, in spaces */
    size_t              nest_indent;
};

/**
 * Check if an output format is valid.
 *
 * @param format    The format to check.
 *
 * @return True if the format is valid, false otherwise.
 */
static inline bool
aushape_format_is_valid(const struct aushape_format *format)
{
    return format != NULL &&
           aushape_lang_is_valid(format->lang);
}

#endif /* _AUSHAPE_FORMAT_H */
