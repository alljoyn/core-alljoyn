/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
#include <iostream>
#include "GuidUtil.h"
#include <alljoyn/AboutData.h>

static const char versionPreamble[] = "ConfigService version: 1\n"
                                      "Copyright AllSeen Alliance.\n";

using namespace ajn;
using namespace services;

OptParser::OptParser(int argc, char** argv) :
    argc(argc), argv(argv) {
    factoryConfigFile.assign("FactoryConfigService.conf");
    configFile.assign("ConfigService.conf");
}

qcc::String OptParser::GetFactoryConfigFile() const {
    return factoryConfigFile;
}

qcc::String OptParser::GetConfigFile() const {
    return configFile;
}

qcc::String OptParser::GetAppId() const {
    return appGUID;
}

void OptParser::PrintUsage() {
    qcc::String cmd = argv[0];
    cmd = cmd.substr(cmd.find_last_of('/') + 1);

    std::cerr << cmd.c_str() << " [--factory-config-file=FILE | --config-file=FILE |  --appId=APPID"
        "]\n"

        "    --factory-config-file=FILE\n"
        "        Configuration file with factory settings.\n\n"
        "    --config-file=FILE\n"
        "        Active configuration file that persists user's updates\n\n"
        "    --appId=\n"
        "        Use the specified it is HexString of 16 bytes (32 chars) \n\n"
        "    --version\n"
        "        Print the version and copyright string, and exit." << std::endl;
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

    if (argc == 1) {

        return result;
    }

    int indx;
    for (indx = 1; indx < argc; indx++) {
        qcc::String arg(argv[indx]);
        if (arg.compare("--version") == 0) {
            std::cout << versionPreamble << std::endl;
            result = PR_EXIT_NO_ERROR;
            break;
        } else if (arg.compare(0, sizeof("--appId") - 1, "--appId") == 0) {
            appGUID = arg.substr(sizeof("--appId"));
            if ((appGUID.length() != 32) || (!IsAllHex(appGUID.c_str()))) {
                result = PR_INVALID_APPID;
                std::cerr << "Invalid appId: \"" << argv[indx] << "\"" << std::endl;
                break;
            }
        } else if (arg.compare(0, sizeof("--factory-config-file") - 1, "--factory-config-file") == 0) {
            factoryConfigFile = arg.substr(sizeof("--factory-config-file"));
        } else if (arg.compare(0, sizeof("--config-file") - 1, "--config-file") == 0) {
            configFile = arg.substr(sizeof("--config-file"));
        } else if ((arg.compare("--help") == 0) || (arg.compare("-h") == 0)) {
            PrintUsage();
            result = PR_EXIT_NO_ERROR;
            break;
        } else {
            result = PR_INVALID_OPTION;
            std::cerr << "Invalid option: \"" << argv[indx] << "\"" << std::endl;
            break;
        }
    }
    return result;
}
