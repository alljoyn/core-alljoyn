/******************************************************************************
 * Copyright (c) 2013 - 2014, AllSeen Alliance. All rights reserved.
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

#ifndef OPTPARSER_H_
#define OPTPARSER_H_

#include <alljoyn/about/AboutServiceApi.h>

/**
 * Class to parse arguments
 */
class OptParser {
  public:
    enum ParseResultCode {
        PR_OK, PR_EXIT_NO_ERROR, PR_INVALID_OPTION, PR_MISSING_OPTION, PR_INVALID_APPID
    };

    OptParser(int argc, char** argv);

    ParseResultCode ParseResult();

    qcc::String GetAppId() const;

    qcc::String GetDeviceId() const;

    qcc::String GetDefaultLanguage() const;

    int GetPort() const;

  private:
    int argc;

    char** argv;

    bool IsAllHex(const char* data);

    void PrintUsage();

    qcc::String appGUID;

    qcc::String defaultLanguage;

    qcc::String deviceId;

    int port;
};

#endif /* OPTPARSER_H_ */
