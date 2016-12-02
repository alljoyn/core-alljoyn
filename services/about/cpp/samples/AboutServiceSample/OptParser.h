/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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