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

#include <qcc/String.h>

/*
 *
 */
class OptParser {
  public:
    enum ParseResultCode {
        PR_OK, PR_EXIT_NO_ERROR, PR_OPTION_CONFLICT, PR_INVALID_OPTION, PR_MISSING_OPTION, PR_INVALID_APPID

    };

    OptParser(int argc, char** argv);

    ParseResultCode ParseResult();

    qcc::String GetConfigFile() const;

    qcc::String GetDaemonSpec() const;

    qcc::String GetAppID() const;

    int GetPort() const;

    bool GetInternal() const;

  private:
    int argc;
    char** argv;

    bool internal;
    bool IsAllHex(const char* data);
    void PrintUsage();

    qcc::String configFile;
    qcc::String daemonSpec;
    qcc::String appGUID;
    int port;
};

#endif /* OPTPARSER_H_ */