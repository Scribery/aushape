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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

void
usage(FILE *stream)
{
    fprintf(stream,
            "Usage: aushape FORMAT\n"
            "Convert raw audit log to XML or JSON\n"
            "\n"
            "Arguments:\n"
            "    FORMAT   Output format (\"XML\" or \"JSON\")\n"
            "\n");
}

struct conv_output_data {
    /** True if output an event already */
    bool got_event;
};

bool conv_output_fn(enum aushape_format format, const char *ptr, size_t len,
                    void *abstract_data)
{
    struct conv_output_data *data = (struct conv_output_data *)abstract_data;

    if (data->got_event && format == AUSHAPE_FORMAT_JSON) {
        write(STDOUT_FILENO, ",", 1);
    }
    write(STDOUT_FILENO, "\n", 1);
    write(STDOUT_FILENO, ptr, len);
    data->got_event = true;
    return true;
}

int
main(int argc, char **argv)
{
    int status = 1;
    enum aushape_format format;
    struct conv_output_data conv_output_data = {.got_event = false};
    struct aushape_conv *conv = NULL;
    enum aushape_conv_rc conv_rc;
    char buf[4096];
    const char *str;
    ssize_t rc;

    if (argc != 2) {
        fprintf(stderr, "Output format not specified\n");
        usage(stderr);
        goto cleanup;
    }

    if (strcasecmp(argv[1], "XML") == 0) {
        format = AUSHAPE_FORMAT_XML;
    } else if (strcasecmp(argv[1], "JSON") == 0) {
        format = AUSHAPE_FORMAT_JSON;
    } else {
        fprintf(stderr, "Invalid output format: %s\n", argv[1]);
        usage(stderr);
        goto cleanup;
    }

    conv_rc = aushape_conv_create(&conv, format,
                                  conv_output_fn, &conv_output_data);
    if (conv_rc != AUSHAPE_CONV_RC_OK) {
        fprintf(stderr, "Failed creating converter: %s\n",
                aushape_conv_rc_to_desc(conv_rc));
    }

    if (format == AUSHAPE_FORMAT_XML) {
        str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
              "<log>";
    } else {
        str = "[";
    }
    write(STDOUT_FILENO, str, strlen(str));

    while ((rc = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        conv_rc = aushape_conv_input(conv, buf, (size_t)rc);
        if (conv_rc != AUSHAPE_CONV_RC_OK) {
            fprintf(stderr, "Failed feeding the converter: %s\n",
                    aushape_conv_rc_to_desc(conv_rc));
            goto cleanup;
        }
    }

    conv_rc = aushape_conv_flush(conv);
    if (conv_rc != AUSHAPE_CONV_RC_OK) {
        fprintf(stderr, "Failed flushing the converter: %s\n",
                aushape_conv_rc_to_desc(conv_rc));
        goto cleanup;
    }

    if (format == AUSHAPE_FORMAT_XML) {
        str = "\n</log>\n";
    } else {
        str = "\n]\n";
    }
    write(STDOUT_FILENO, str, strlen(str));

    if (rc < 0) {
        fprintf(stderr, "Failed reading stdin: %s\n", strerror(errno));
        goto cleanup;
    }

    status = 0;

cleanup:

    aushape_conv_destroy(conv);
    return status;
}
