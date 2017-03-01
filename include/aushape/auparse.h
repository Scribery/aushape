/**
 * @brief Substitutes for possibly missing auparse functions
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

#ifndef _AUSHAPE_AUPARSE_H
#define _AUSHAPE_AUPARSE_H

#include <auparse.h>

/**
 * Get name of the type of the record currently being parsed by an auparse
 * instance.
 *
 * @param au    The auparse instance to get the current record type name for.
 *
 * @return The current record type name, or NULL on (unspecified) error.
 */
const char *aushape_auparse_get_type_name(auparse_state_t *au);

#endif /* _AUSHAPE_AUPARSE_H */
