/**
 * @file
 *
 * This file implements methods from the ERConfig class.
 */

/******************************************************************************
 *
 *
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#include <qcc/platform.h>
#include <map>
#include <stdio.h>

#include <qcc/Debug.h>
#include <qcc/Config.h>
#include <qcc/Environ.h>
#include <qcc/FileStream.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <Status.h>

#define QCC_MODULE "CONFIG"

using namespace std;
using namespace qcc;

Config::Config(void)
{
    /** Config file name */

    String iniFileResolved = "ER_INI.dat";

    Environ* env = Environ::GetAppEnviron();

    String dir = env->Find("splicehomedir");
    if (dir.empty()) {
        dir = env->Find("SPLICEHOMEDIR");
    }

#if !defined(NDEBUG)
    // Allow testing using a config file in the current directory.
    // Env var value is insignificant, just existence.
    if (env->Find("SPLICECONFIGINCURRENTDIR").empty() && !dir.empty()) {
        iniFileResolved = dir + "/" + iniFileResolved;
    }
#endif

    FileSource iniSource(iniFileResolved);

    if (!iniSource.IsValid()) {
        QCC_LogError(ER_FAIL, ("Unable to open config file %s", iniFileResolved.c_str()));
        // use defaults...
        // ...
    } else {
        String line;
        while (ER_OK == iniSource.GetLine(line)) {
            size_t pos = line.find_first_of(';');
            if (String::npos != pos) {
                line = line.substr(0, pos);
            }
            pos = line.find_first_of('=');
            if (String::npos != pos && (line.length() >= pos + 2)) {
                String key = Trim(line.substr(0, pos));
                String val = Trim(line.substr(pos + 1, String::npos));
                nameValuePairs[key] = val;
            }
            line.clear();
        }
    }
}