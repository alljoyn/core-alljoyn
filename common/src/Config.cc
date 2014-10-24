/**
 * @file
 *
 * This file implements methods from the ERConfig class.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2011, 2014 AllSeen Alliance. All rights reserved.
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
