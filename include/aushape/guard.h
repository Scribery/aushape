/**
 * @brief Error handling macros
 *
 * All the macros check for a condition and if it fails, go to label named
 * "cleanup". Before that they assign an error code to a variable named "rc",
 * which is assumed to be declared as enum aushape_rc.
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

#ifndef _AUSHAPE_GUARD_H
#define _AUSHAPE_GUARD_H

#include <aushape/rc.h>

/**
 * Exit with the specified return code if a bool expression is false.
 * To exit stores return code in "rc" variable and goes to "cleanup" label.
 *
 * @param _RC_TOKEN     The return code name without the AUSHAPE_RC_
 *                      prefix.
 * @param _bool_expr    The bool expression to evaluate.
 */
#define AUSHAPE_GUARD_BOOL(_RC_TOKEN, _bool_expr) \
    do {                                            \
        if (!(_bool_expr)) {                        \
            rc = AUSHAPE_RC_##_RC_TOKEN;            \
            goto cleanup;                           \
        }                                           \
    } while (0)

/**
 * If an expression producing return code doesn't evaluate to AUSHAPE_RC_OK,
 * then exit with the evaluated return code. Stores the evaluated return code
 * in "rc" variable, goes to "cleanup" label to exit.
 *
 * @param _rc_expr  The return code expression to evaluate.
 */
#define AUSHAPE_GUARD_RC(_rc_expr) \
    do {                            \
        rc = (_rc_expr);            \
        if (rc != AUSHAPE_RC_OK) {  \
            goto cleanup;           \
        }                           \
    } while (0)

#endif /* _AUSHAPE_GUARD_H */
