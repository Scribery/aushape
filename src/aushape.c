/*
 * Copyright (C) 2016 Red Hat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>
#include <aushape/conf.h>
#include <aushape/conv.h>
#include <aushape/fd_output.h>
#include <aushape/syslog_output.h>
#include <aushape/syslog_misc.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

/**
 * Create a converter attached to an output according to configuration.
 *
 * @param pconv Location for the created converter pointer.
 * @param conf  The aushape configuration.
 *
 * @return True if converter created succesfully, false if creation failed,
 *         and an error message was printed to stderr.
 */
static bool
create_converter(struct aushape_conv **pconv,
                 const struct aushape_conf *conf)
{
    bool result = false;
    int output_fd = -1;
    bool output_fd_owned = false;
    struct aushape_output *output = NULL;
    struct aushape_conv *conv = NULL;
    enum aushape_rc rc;

    /* Create output */
    if (conf->output_type == AUSHAPE_CONF_OUTPUT_TYPE_FD) {
        if (strcmp(conf->output_conf.fd.path, "-") == 0) {
            output_fd = STDOUT_FILENO;
        } else {
            output_fd = open(conf->output_conf.fd.path,
                             O_CREAT | O_TRUNC | O_WRONLY,
                             S_IRUSR | S_IRGRP | S_IROTH |
                             S_IWUSR | S_IWGRP | S_IWOTH);
            if (output_fd < 0) {
                fprintf(stderr, "Failed opening output file \"%s\": %s\n",
                        conf->output_conf.fd.path, strerror(errno));
                goto cleanup;
            }
            output_fd_owned = true;
        }
        rc = aushape_fd_output_create(&output, output_fd, output_fd_owned);
        if (rc != AUSHAPE_RC_OK) {
            fprintf(stderr, "Failed creating output: %s\n",
                    aushape_rc_to_desc(rc));
            goto cleanup;
        }
        output_fd_owned = false;
    } else if (conf->output_type == AUSHAPE_CONF_OUTPUT_TYPE_SYSLOG) {
        openlog("aushape", LOG_NDELAY, conf->output_conf.syslog.facility);
        rc = aushape_syslog_output_create(&output,
                                          conf->output_conf.syslog.priority);
        if (rc != AUSHAPE_RC_OK) {
            fprintf(stderr, "Failed creating output: %s\n",
                    aushape_rc_to_desc(rc));
            goto cleanup;
        }
    } else {
        fprintf(stderr, "Unknown output type: %u\n",
                (unsigned int)conf->output_type);
        goto cleanup;
    }

    /* Create converter */
    rc = aushape_conv_create(&conv, &conf->format, output, true);
    if (rc != AUSHAPE_RC_OK) {
        fprintf(stderr, "Failed creating converter: %s\n",
                aushape_rc_to_desc(rc));
        goto cleanup;
    }
    output = NULL;

    /* Output converter */
    *pconv = conv;
    conv = NULL;
    result = true;
cleanup:

    if (output_fd_owned) {
        close(output_fd);
    }
    aushape_output_destroy(output);
    aushape_conv_destroy(conv);
    return result;
}

int
main(int argc, char **argv)
{
    int status = 1;
    struct aushape_conf conf;
    bool input_fd_owned = false;
    int input_fd;
    struct aushape_conv *conv = NULL;
    enum aushape_rc aushape_rc;
    char buf[4096];
    ssize_t rc;

    /* Read configuration */
    if (!aushape_conf_load(&conf, argc, argv)) {
        goto cleanup;
    }

    /* If asked for help */
    if (conf.help) {
        fprintf(stdout, "%s\n", aushape_conf_cmd_help);
        status = 0;
        goto cleanup;
    }

    /* If asked for version */
    if (conf.version) {
        fprintf(stdout, "%s",
                PACKAGE_STRING "\n"
                "Copyright (C) 2016 Red Hat\n"
                "License GPLv2+: GNU GPL version 2 or later "
                    "<http://gnu.org/licenses/gpl.html>.\n"
                "\n"
                "This is free software: "
                    "you are free to change and redistribute it.\n"
                "There is NO WARRANTY, to the extent permitted by law.\n");
        status = 0;
        goto cleanup;
    }

    /* Open input */
    if (strcmp(conf.input, "-") == 0) {
        input_fd = STDIN_FILENO;
    } else {
        input_fd = open(conf.input, O_RDONLY);
        if (input_fd < 0) {
            fprintf(stderr, "Failed opening input file \"%s\": %s\n",
                    conf.input, strerror(errno));
            goto cleanup;
        }
        input_fd_owned = true;
    }

    /* Create converter */
    if (!create_converter(&conv, &conf)) {
        goto cleanup;
    }

    aushape_rc = aushape_conv_begin(conv);
    if (aushape_rc != AUSHAPE_RC_OK) {
        fprintf(stderr, "Failed starting document: %s\n",
                aushape_rc_to_desc(aushape_rc));
        goto cleanup;
    }

    while ((rc = read(input_fd, buf, sizeof(buf))) > 0) {
        aushape_rc = aushape_conv_input(conv, buf, (size_t)rc);
        if (aushape_rc != AUSHAPE_RC_OK) {
            fprintf(stderr, "Failed feeding the converter: %s\n",
                    aushape_rc_to_desc(aushape_rc));
            goto cleanup;
        }
    }

    aushape_rc = aushape_conv_flush(conv);
    if (aushape_rc != AUSHAPE_RC_OK) {
        fprintf(stderr, "Failed flushing the converter: %s\n",
                aushape_rc_to_desc(aushape_rc));
        goto cleanup;
    }

    aushape_rc = aushape_conv_end(conv);
    if (aushape_rc != AUSHAPE_RC_OK) {
        fprintf(stderr, "Failed finishing document: %s\n",
                aushape_rc_to_desc(aushape_rc));
        goto cleanup;
    }

    if (rc < 0) {
        fprintf(stderr, "Failed reading input: %s\n", strerror(errno));
        goto cleanup;
    }

    status = 0;

cleanup:
    aushape_conv_destroy(conv);
    if (input_fd_owned) {
        close(input_fd);
    }
    return status;
}
