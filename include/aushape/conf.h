/**
 * @brief Aushape configuration management
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

#ifndef _AUSHAPE_CONF_H
#define _AUSHAPE_CONF_H

#include <aushape/format.h>
#include <stdbool.h>

/** Command-line usage help message */
extern const char *aushape_conf_cmd_help;

/** Output type */
enum aushape_conf_output_type {
    AUSHAPE_CONF_OUTPUT_TYPE_INVALID,
    AUSHAPE_CONF_OUTPUT_TYPE_FD,
    AUSHAPE_CONF_OUTPUT_TYPE_SYSLOG,
    AUSHAPE_CONF_OUTPUT_TYPE_NUM,
};

static inline bool
aushape_conf_output_type_is_valid(enum aushape_conf_output_type type)
{
    return type > AUSHAPE_CONF_OUTPUT_TYPE_INVALID &&
           type < AUSHAPE_CONF_OUTPUT_TYPE_NUM;
}

/** FD output configuration */
struct aushape_conf_fd_output {
    /** Output file name, or "-" */
    const char *path;
};

/** Syslog output configuration */
struct aushape_conf_syslog_output {
    /** Syslog facility */
    int facility;
    /** Syslog priority */
    int priority;
};

/** Configuration */
struct aushape_conf {
    /** True if -h/--help option was specified */
    bool                                help;
    /** True if -v/--version option was specified */
    bool                                version;
    /** Input file name, or "-" */
    const char                         *input;
    /** Output format */
    struct aushape_format               format;
    /** Output type */
    enum aushape_conf_output_type       output_type;
    struct {
        /* FD output configuration */
        struct aushape_conf_fd_output       fd;
        /* Syslog output configuration */
        struct aushape_conf_syslog_output   syslog;
    } output_conf;
};

/**
 * Load aushape configuration from the command line arguments.
 *
 * @param pconf     Location for the loaded configuration.
 *                  Not modified in case of error.
 * @param argc      The argc argument to main.
 * @param argv      The argv argument to main.
 *
 * @return True if loaded successfully, false if failed and an error message,
 *         followed by a help message, if necessary, were printed to stderr.
 */
extern bool aushape_conf_load(struct aushape_conf *pconf,
                              int argc, char **argv);

#endif /* _AUSHAPE_CONF_H */
