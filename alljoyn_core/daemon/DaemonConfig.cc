/**
 * @file
 *
 * Configuration helper
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringSource.h>
#include <qcc/StringUtil.h>

#include <alljoyn/Status.h>

#include "DaemonConfig.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;
using namespace ajn;

DaemonConfig* DaemonConfig::singleton = NULL;

DaemonConfig::DaemonConfig() : config(NULL)
{
}

DaemonConfig::~DaemonConfig()
{
    delete config;
}

DaemonConfig* DaemonConfig::Load(qcc::Source& configSrc)
{
    if (!singleton) {
        singleton = new DaemonConfig();
    }
    XmlParseContext xmlParseCtx(configSrc);

    if (singleton->config) {
        delete singleton->config;
        singleton->config = NULL;
    }

    QStatus status = XmlElement::Parse(xmlParseCtx);
    if (status == ER_OK) {
        singleton->config = xmlParseCtx.DetachRoot();
    } else {
        delete singleton;
        singleton = NULL;
    }
    return singleton;
}

DaemonConfig* DaemonConfig::Load(const char* configXml)
{
    qcc::StringSource src(configXml);
    return Load(src);
}

uint32_t DaemonConfig::Get(const char* key, uint32_t defaultVal)
{
    return StringToU32(Get(key), 10, defaultVal);
}

qcc::String DaemonConfig::Get(const char* key, const char* defaultVal)
{
    qcc::String path = key;
    std::vector<const XmlElement*> elems = config->GetPath(path);
    if (elems.size() > 0) {
        size_t pos = path.find_first_of('@');
        if (pos != String::npos) {
            return elems[0]->GetAttribute(path.substr(pos + 1));
        } else {
            return elems[0]->GetContent();
        }
    }
    return defaultVal ? defaultVal : "";
}

std::vector<qcc::String> DaemonConfig::GetList(const char* key)
{
    std::vector<qcc::String> result;
    std::vector<const XmlElement*> elems = config->GetPath(key);
    for (size_t i = 0; i < elems.size(); ++i) {
        result.push_back(elems[i]->GetContent());
    }
    return result;
}

bool DaemonConfig::Has(const char* key)
{
    String path = key;
    return config ? !config->GetPath(path).empty() : false;
}
