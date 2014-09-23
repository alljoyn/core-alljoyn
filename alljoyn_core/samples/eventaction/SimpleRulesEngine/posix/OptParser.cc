/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#include "OptParser.h"
#include <fstream>

static const char versionPreamble[] = "AboutService version: %s\n"
                                      "Copyright (c) 2014 AllSeenAlliance.\n";

using namespace ajn;

OptParser::OptParser(int argc, char** argv) :
    argc(argc), argv(argv), internal(false) {
    port = 900;
    appGUID.assign("");
    srand(time(NULL));
    for (int i = 0; i < 32; i++) {
        /* Rand val from 0-9 */
        appGUID += '0' + rand() % 10;
    }
}

qcc::String OptParser::GetConfigFile() const {
    return configFile;
}

qcc::String OptParser::GetDaemonSpec() const {
    return daemonSpec;
}
qcc::String OptParser::GetAppID() const {
    return appGUID;
}

int OptParser::GetPort() const {
    return port;
}

bool OptParser::GetInternal() const {
    return internal;
}

void OptParser::PrintUsage() {
    qcc::String cmd = argv[0];
    cmd = cmd.substr(cmd.find_last_of('/') + 1);

    fprintf(stderr, "%s [--port|  | --config-file=FILE |  --daemonspec | --appid"
            "]\n"
            "    --daemonspec=\n"
            "       daemon spec used by the service.\n\n"
            "    --port=\n"
            "        used to bind the service.\n\n"
            "    --config-file=FILE\n"
            "        Use the specified configuration file.\n\n"
            "    --appid=\n"
            "        Use the specified it is HexString of 16 bytes (32 chars) \n\n"
            "    --version\n"
            "        Print the version and copyright string, and exit.\n", cmd.c_str());
}

bool OptParser::IsAllHex(const char* data) {

    for (size_t index = 0; index < strlen(data); ++index) {
        if (!isxdigit(data[index])) {
            return false;
        }

    }
    return true;

}

OptParser::ParseResultCode OptParser::ParseResult() {
    ParseResultCode result = PR_OK;
    int i = 0;

    if (argc == 1) {
        internal = true;
        goto exit;
    }

    for (i = 1; i < argc; ++i) {
        qcc::String arg(argv[i]);
        if (arg.compare("--version") == 0) {
            printf(versionPreamble, "1");
            result = PR_EXIT_NO_ERROR;
            goto exit;
        } else if (arg.compare(0, sizeof("--port") - 1, "--port") == 0) {
            port = atoi(arg.substr(sizeof("--port")).c_str());
        } else if (arg.compare(0, sizeof("--daemonspec") - 1, "--daemonspec") == 0) {
            daemonSpec = arg.substr(sizeof("--daemonspec"));
        } else if (arg.compare(0, sizeof("--appid") - 1, "--appid") == 0) {
            appGUID = arg.substr(sizeof("--appid"));
            if ((appGUID.length() != 32) || (!IsAllHex(appGUID.c_str()))) {
                result = PR_INVALID_APPID;
                goto exit;
            }
        } else if (arg.compare("--config-file") == 0) {
            if (!configFile.empty() || internal) {
                result = PR_OPTION_CONFLICT;
                goto exit;
            }
            ++i;
            if (i == argc) {
                result = PR_MISSING_OPTION;
                goto exit;
            }
            configFile = argv[i];
        } else if (arg.compare(0, sizeof("--config-file") - 1, "--config-file") == 0) {
            if (!configFile.empty() || internal) {
                result = PR_OPTION_CONFLICT;
                goto exit;
            }
            configFile = arg.substr(sizeof("--config-file"));
        } else if ((arg.compare("--help") == 0) || (arg.compare("-h") == 0)) {
            PrintUsage();
            result = PR_EXIT_NO_ERROR;
            goto exit;
        } else {
            result = PR_INVALID_OPTION;
            goto exit;
        }
    }

exit:

    internal = configFile.empty();

    switch (result) {
    case PR_OPTION_CONFLICT:
        fprintf(stderr, "Option \"%s\" is in conflict with a previous option.\n", argv[i]);
        break;

    case PR_INVALID_APPID:
        fprintf(stderr, "Invalid appid: \"%s\"\n", argv[i]);
        break;

    case PR_INVALID_OPTION:
        fprintf(stderr, "Invalid option: \"%s\"\n", argv[i]);
        break;

    case PR_MISSING_OPTION:
        fprintf(stderr, "No config file specified.\n");
        PrintUsage();
        break;

    default:
        break;
    }
    return result;
}
