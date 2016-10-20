/**
 * @brief An aushape raw audit log converter return code
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

#ifndef _AUSHAPE_CONV_RC_H
#define _AUSHAPE_CONV_RC_H

/** Return codes */
enum aushape_conv_rc {
    /** Execution succeeded */
    AUSHAPE_CONV_RC_OK  = 0,
    /** Invalid arguments supplied to a function */
    AUSHAPE_CONV_RC_INVALID_ARGS,
    /** Object is in invalid state */
    AUSHAPE_CONV_RC_INVALID_STATE,
    /** Memory allocation failed */
    AUSHAPE_CONV_RC_NOMEM,
    /** Output function failed */
    AUSHAPE_CONV_RC_OUTPUT_FAILED,
    /** An underlying auparse call failed */
    AUSHAPE_CONV_RC_AUPARSE_FAILED,
    /** Invalid execve record sequence encountered */
    AUSHAPE_CONV_RC_INVALID_EXECVE,
    /** Number of return codes (not a valid return code) */
    AUSHAPE_CONV_RC_NUM
};

/**
 * Retrieve the name of a return code.
 *
 * @param rc    The return code to retrieve the name for.
 *
 * @return A constant string with the name of the return code.
 */
extern const char *aushape_conv_rc_to_name(enum aushape_conv_rc rc);

/**
 * Retrieve the description of a return code.
 *
 * @param rc    The return code to describe.
 *
 * @return A constant string describing the return code.
 */
extern const char *aushape_conv_rc_to_desc(enum aushape_conv_rc rc);

#endif /* _AUSHAPE_CONV_RC_H */
