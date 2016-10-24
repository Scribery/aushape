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

#include <aushape/conv.h>
#include <aushape/fd_output.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

void
usage(FILE *stream)
{
    fprintf(stream,
            "Usage: aushape LANG\n"
            "Convert raw audit log to XML or JSON\n"
            "\n"
            "Arguments:\n"
            "    LANG   Output language (\"XML\" or \"JSON\")\n"
            "\n");
}

bool conv_output_fn(const char *ptr, size_t len,
                    void *abstract_data)
{
    (void)abstract_data;
    write(STDOUT_FILENO, ptr, len);
    return true;
}

int
main(int argc, char **argv)
{
    int status = 1;
    struct aushape_format format = {.fold_level = 5,
                                    .init_indent = 0,
                                    .nest_indent = 4};
    struct aushape_output *output = NULL;
    struct aushape_conv *conv = NULL;
    enum aushape_rc aushape_rc;
    char buf[4096];
    ssize_t rc;

    if (argc != 2) {
        fprintf(stderr, "Output language not specified\n");
        usage(stderr);
        goto cleanup;
    }

    if (strcasecmp(argv[1], "XML") == 0) {
        format.lang = AUSHAPE_LANG_XML;
    } else if (strcasecmp(argv[1], "JSON") == 0) {
        format.lang = AUSHAPE_LANG_JSON;
    } else {
        fprintf(stderr, "Invalid output language: %s\n", argv[1]);
        usage(stderr);
        goto cleanup;
    }

    aushape_rc = aushape_fd_output_create(&output, STDOUT_FILENO, false);
    if (aushape_rc != AUSHAPE_RC_OK) {
        fprintf(stderr, "Failed creating output: %s\n",
                aushape_rc_to_desc(aushape_rc));
        goto cleanup;
    }

    aushape_rc = aushape_conv_create(&conv, SSIZE_MAX, &format,
                                     output, false);
    if (aushape_rc != AUSHAPE_RC_OK) {
        fprintf(stderr, "Failed creating converter: %s\n",
                aushape_rc_to_desc(aushape_rc));
        goto cleanup;
    }

    aushape_rc = aushape_conv_begin(conv);
    if (aushape_rc != AUSHAPE_RC_OK) {
        fprintf(stderr, "Failed starting document: %s\n",
                aushape_rc_to_desc(aushape_rc));
        goto cleanup;
    }

    while ((rc = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
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
        fprintf(stderr, "Failed reading stdin: %s\n", strerror(errno));
        goto cleanup;
    }

    status = 0;

cleanup:

    aushape_conv_destroy(conv);
    aushape_output_destroy(output);
    return status;
}
