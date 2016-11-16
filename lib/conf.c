/*
 * Aushape configuration management
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

#include <aushape/conf.h>
#include <aushape/syslog_misc.h>
#include <aushape/misc.h>
#include <syslog.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

const char *aushape_conf_cmd_help =
   "Usage: aushape [OPTION]... [INPUT]\n"
   "Convert audit log to JSON or XML.\n"
   "\n"
   "Arguments:\n"
   "    INPUT                   Input file path or \"-\" for stdin.\n"
   "                            Default: \"-\"\n"
   "\n"
   "General options:\n"
   "    -h, --help              Output this help message and exit.\n"
   "    -v, --version           Output version information and exit.\n"
   "\n"
   "Formatting options:\n"
   "    -l, --lang=STRING       Output STRING language (\"xml\" or \"json\").\n"
   "                            Default: \"json\"\n"
   "    --events-per-doc=STRING Put STRING amount of events into each document:\n"
   "                                0 / \"none\"  - don't put events in documents,\n"
   "                                N           - N events per document max,\n"
   "                                -N          - N (floor) bytes per document max,\n"
   "                                \"all\"       - all events in one document.\n"
   "                            Default: \"all\"\n"
   "    --fold=STRING           Fold STRING nesting level into single line:\n"
   "                                0 / \"all\"   - fold all, single-line output,\n"
   "                                N           - fold at level N,\n"
   "                                \"none\"      - unfold fully.\n"
   "                            Default: 5\n"
   "    --indent=NUMBER         Indent each nesting level by NUMBER spaces.\n"
   "                            Default: 4\n"
   "    --with-raw              Include original raw log messages in the output.\n"
   "                            Default: off\n"
   "\n"
   "Output options:\n"
   "    -o, --output=STRING         Use STRING output type (\"file\"/\"syslog\").\n"
   "                                Default: \"file\"\n"
   "    -f,--file=PATH              Write to file PATH with file output.\n"
   "                                Write to stdout if PATH is \"-\"\n"
   "                                Default: \"-\"\n"
   "    --syslog-facility=STRING    Log with STRING facility with syslog output.\n"
   "                                Default: \"authpriv\"\n"
   "    --syslog-priority=STRING    Log with STRING priority with syslog output.\n"
   "                                Default: \"info\"\n";

/** Option codes */
enum aushape_conf_opt {
    AUSHAPE_CONF_OPT_HELP = 'h',
    AUSHAPE_CONF_OPT_VERSION = 'v',
    AUSHAPE_CONF_OPT_LANG = 'l',
    AUSHAPE_CONF_OPT_OUTPUT = 'o',
    AUSHAPE_CONF_OPT_FILE = 'f',
    AUSHAPE_CONF_OPT_EVENTS_PER_DOC = 0x100,
    AUSHAPE_CONF_OPT_FOLD,
    AUSHAPE_CONF_OPT_INDENT,
    AUSHAPE_CONF_OPT_WITH_RAW,
    AUSHAPE_CONF_OPT_SYSLOG_FACILITY,
    AUSHAPE_CONF_OPT_SYSLOG_PRIORITY,
};

/** Description of short options */
static const char *aushape_conf_shortopts = ":hvl:o:f:";

/** Description of long options */
static const struct option aushape_conf_longopts[] = {
    {
        .name = "help",
        .val = AUSHAPE_CONF_OPT_HELP,
        .has_arg = no_argument,
    },
    {
        .name = "version",
        .val = AUSHAPE_CONF_OPT_VERSION,
        .has_arg = no_argument,
    },
    {
        .name = "lang",
        .val = AUSHAPE_CONF_OPT_LANG,
        .has_arg = required_argument,
    },
    {
        .name = "output",
        .val = AUSHAPE_CONF_OPT_OUTPUT,
        .has_arg = required_argument,
    },
    {
        .name = "file",
        .val = AUSHAPE_CONF_OPT_FILE,
        .has_arg = required_argument,
    },
    {
        .name = "events-per-doc",
        .val = AUSHAPE_CONF_OPT_EVENTS_PER_DOC,
        .has_arg = required_argument,
    },
    {
        .name = "fold",
        .val = AUSHAPE_CONF_OPT_FOLD,
        .has_arg = required_argument,
    },
    {
        .name = "indent",
        .val = AUSHAPE_CONF_OPT_INDENT,
        .has_arg = required_argument,
    },
    {
        .name = "with-raw",
        .val = AUSHAPE_CONF_OPT_WITH_RAW,
        .has_arg = no_argument,
    },
    {
        .name = "syslog-facility",
        .val = AUSHAPE_CONF_OPT_SYSLOG_FACILITY,
        .has_arg = required_argument,
    },
    {
        .name = "syslog-priority",
        .val = AUSHAPE_CONF_OPT_SYSLOG_PRIORITY,
        .has_arg = required_argument,
    },
    {
        .name = NULL
    }
};

bool
aushape_conf_load(struct aushape_conf *pconf, int argc, char **argv)
{
    bool result = false;
    struct aushape_conf conf = {
        .input = "-",
        .format = {
            .lang = AUSHAPE_LANG_JSON,
            .fold_level = 5,
            .init_indent = 0,
            .nest_indent = 4,
            .events_per_doc = SSIZE_MAX,
            .with_raw = false,
        },
        .output_type = AUSHAPE_CONF_OUTPUT_TYPE_FD,
        .output_conf = {
            .fd = {
                .path = "-"
            },
            .syslog = {
                .facility = LOG_AUTHPRIV,
                .priority = LOG_INFO,
            }
        }
    };
    int opterr_orig;
    int optind_orig;
    int optcode;
    int end;
    int i;

    /* Ask getopt_long to not print an error message */
    opterr_orig = opterr;
    opterr = 0;

    /* Ask getopt_long to start from the first argument */
    optind_orig = optind;
    optind = 1;

    /* While there are options */
    while ((optcode = getopt_long(argc, argv,
                                  aushape_conf_shortopts,
                                  aushape_conf_longopts, NULL)) >= 0) {
        switch (optcode) {
        case AUSHAPE_CONF_OPT_HELP:
            conf.help = true;
            break;

        case AUSHAPE_CONF_OPT_VERSION:
            conf.version = true;
            break;

        case AUSHAPE_CONF_OPT_LANG:
            if (strcasecmp(optarg, "json") == 0) {
                conf.format.lang = AUSHAPE_LANG_JSON;
            } else if (strcasecmp(optarg, "xml") == 0) {
                conf.format.lang = AUSHAPE_LANG_XML;
            } else {
                fprintf(stderr, "Invalid language: %s\n%s\n",
                        optarg, aushape_conf_cmd_help);
                goto cleanup;
            }
            break;

        case AUSHAPE_CONF_OPT_OUTPUT:
            if (strcasecmp(optarg, "file") == 0) {
                conf.output_type = AUSHAPE_CONF_OUTPUT_TYPE_FD;
            } else if (strcasecmp(optarg, "syslog") == 0) {
                conf.output_type = AUSHAPE_CONF_OUTPUT_TYPE_SYSLOG;
            } else {
                fprintf(stderr, "Invalid output type: %s\n%s\n",
                        optarg, aushape_conf_cmd_help);
                goto cleanup;
            }
            break;

        case AUSHAPE_CONF_OPT_FILE:
            conf.output_conf.fd.path = optarg;
            break;

        case AUSHAPE_CONF_OPT_EVENTS_PER_DOC:
            end = 0;
            if (strcasecmp(optarg, "none") == 0) {
                conf.format.events_per_doc = 0;
            } else if (strcasecmp(optarg, "all") == 0) {
                conf.format.events_per_doc = SSIZE_MAX;
            } else if (sscanf(optarg, "%zd%n",
                              &conf.format.events_per_doc, &end) < 1 ||
                       (size_t)end != strlen(optarg)) {
                fprintf(stderr, "Invalid events per doc value: %s\n%s\n",
                        optarg, aushape_conf_cmd_help);
                goto cleanup;
            }
            break;

        case AUSHAPE_CONF_OPT_FOLD:
            end = 0;
            if (strcasecmp(optarg, "none") == 0) {
                conf.format.fold_level = SIZE_MAX;
            } else if (strcasecmp(optarg, "all") == 0) {
                conf.format.fold_level = 0;
            } else if (sscanf(optarg, "%zu%n",
                              &conf.format.fold_level, &end) < 1 ||
                       (size_t)end != strlen(optarg)) {
                fprintf(stderr, "Invalid fold level: %s\n%s\n",
                        optarg, aushape_conf_cmd_help);
                goto cleanup;
            }
            break;

        case AUSHAPE_CONF_OPT_INDENT:
            end = 0;
            if (sscanf(optarg, "%zu%n",
                       &conf.format.nest_indent, &end) < 1 ||
                (size_t)end != strlen(optarg)) {
                fprintf(stderr, "Invalid indent size: %s\n%s\n",
                        optarg, aushape_conf_cmd_help);
                goto cleanup;
            }
            break;

        case AUSHAPE_CONF_OPT_WITH_RAW:
            conf.format.with_raw = true;
            break;

        case AUSHAPE_CONF_OPT_SYSLOG_FACILITY:
            i = aushape_syslog_facility_from_str(optarg);
            if (i < 0) {
                fprintf(stderr, "Invalid syslog facility: %s\n%s\n",
                        optarg, aushape_conf_cmd_help);
                goto cleanup;
            }
            conf.output_conf.syslog.facility = i;
            break;

        case AUSHAPE_CONF_OPT_SYSLOG_PRIORITY:
            i = aushape_syslog_priority_from_str(optarg);
            if (i < 0) {
                fprintf(stderr, "Invalid syslog priority: %s\n%s\n",
                        optarg, aushape_conf_cmd_help);
                goto cleanup;
            }
            conf.output_conf.syslog.priority = i;
            break;

        case ':':
            for (i = 0;
                 i < (int)AUSHAPE_ARRAY_SIZE(aushape_conf_longopts);
                 i++) {
                if (aushape_conf_longopts[i].val == optopt) {
                    break;
                }
            }
            if (i < (int)AUSHAPE_ARRAY_SIZE(aushape_conf_longopts)) {
                if (aushape_conf_longopts[i].val < 0x100) {
                    fprintf(stderr,
                            "Option -%c/--%s argument is missing\n%s\n",
                            aushape_conf_longopts[i].val,
                            aushape_conf_longopts[i].name,
                            aushape_conf_cmd_help);
                } else {
                    fprintf(stderr,
                            "Option --%s argument is missing\n%s\n",
                            aushape_conf_longopts[i].name,
                            aushape_conf_cmd_help);
                }
            } else {
                fprintf(stderr,
                        "Option -%c argument is missing\n%s\n",
                        optopt, aushape_conf_cmd_help);
            }
            goto cleanup;

        case '?':
            if (optopt == 0) {
                fprintf(stderr,
                        "Unknown option encountered\n%s\n",
                        aushape_conf_cmd_help);
            } else {
                fprintf(stderr,
                        "Unknown option encountered: -%c\n%s\n",
                        optopt, aushape_conf_cmd_help);
            }
            goto cleanup;
        default:
            fprintf(stderr, "Unknown option code: %d\n", optcode);
            goto cleanup;
        }
    }

    if (optind == (argc - 1)) {
        conf.input = argv[optind];
    } else if (optind != argc) {
        fprintf(stderr, "Invalid number of positional arguments\n%s\n",
                aushape_conf_cmd_help);
        goto cleanup;
    }

    *pconf = conf;
    result = true;
cleanup:
    optind = optind_orig;
    opterr = opterr_orig;
    return result;
}
