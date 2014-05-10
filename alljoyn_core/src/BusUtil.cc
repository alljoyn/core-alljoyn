/**
 * @file
 *
 * This file implements utilities functions
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include <ctype.h>

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include "BusUtil.h"


#define QCC_MODULE "ALLJOYN"


#define MAX_NAME_LEN 256

using namespace qcc;
using namespace std;

namespace ajn {

bool IsLegalUniqueName(const char* str)
{
    if (!str) {
        return false;
    }
    const char* p = str;

    char c = *p++;
    if (c != ':' || !(IsAlphaNumeric(*p) || (*p == '-') || (*p == '_'))) {
        return false;
    }
    p++;

    size_t periods = 0;
    while ((c = *p++)) {
        if (!IsAlphaNumeric(c) && (c != '-') && (c != '_')) {
            if ((c != '.') || (*p == '.') || (*p == 0)) {
                return false;
            }
            periods++;
        }
    }
    return (periods > 0) && ((p - str) <= MAX_NAME_LEN);
}


bool IsLegalBusName(const char* str)
{
    if (!str) {
        return false;
    }
    if (*str == ':') {
        return IsLegalUniqueName(str);
    }
    const char* p = str;
    size_t periods = 0;
    char c = *p++;
    /* Must begin with an alpha character, underscore, or hyphen */
    if (!IsAlpha(c) && (c != '_') && (c != '-')) {
        return false;
    }
    while ((c = *p++) != 0) {
        if (!IsAlphaNumeric(c) && (c != '_') && (c != '-')) {
            if ((c != '.') || (*p == '.') || (*p == 0) || isdigit(*p)) {
                return false;
            }
            periods++;
        }
    }
    return (periods > 0) && ((p - str) <= MAX_NAME_LEN);
}


bool IsLegalObjectPath(const char* str)
{
    if (!str) {
        return false;
    }
    /* Must begin with slash */
    char c = *str++;
    if (c != '/') {
        return false;
    }
    while ((c = *str++) != 0) {
        if (!IsAlphaNumeric(c) && (c != '_')) {
            if ((c != '/') || (*str == '/') || (*str == 0)) {
                return false;
            }
        }
    }
    return true;
}


bool IsLegalInterfaceName(const char* str)
{
    if (!str) {
        return false;
    }
    const char* p = str;

    /* Must begin with an alpha character or underscore */
    char c = *p++;
    if (!IsAlpha(c) && (c != '_')) {
        return false;
    }
    size_t periods = 0;
    while ((c = *p++) != 0) {
        if (!IsAlphaNumeric(c) && (c != '_')) {
            if ((c != '.') || (*p == '.') || (*p == 0)) {
                return false;
            }
            periods++;
        }
    }
    return (periods > 0) && ((p - str) <= MAX_NAME_LEN);
}


bool IsLegalErrorName(const char* str)
{
    return IsLegalInterfaceName(str);
}


bool IsLegalMemberName(const char* str)
{
    if (!str) {
        return false;
    }
    const char* p = str;
    char c = *p++;

    if (!IsAlpha(c) && (c != '_')) {
        return false;
    }
    while ((c = *p++) != 0) {
        if (!IsAlphaNumeric(c) && (c != '_')) {
            return false;
        }
    }
    return (p - str) <= MAX_NAME_LEN;
}


qcc::String BusNameFromObjPath(const char* str)
{
    qcc::String path;

    if (IsLegalObjectPath(str) && (str[1] != 0)) {
        char c = *str++;
        do {
            if (c == '/') {
                c = '.';
            }
            path.push_back(c);
        } while ((c = *str++) != 0);
    }
    return path;
}

QStatus ParseMatch(const qcc::String& match,
                   std::map<qcc::String, qcc::String>& matchMap)
{
    QStatus status = ER_OK;
    size_t pos = 0;
    while (pos < match.length()) {
        size_t endPos = match.find_first_of(',', pos);
        if (endPos == String::npos) {
            endPos = match.length();
        }
        size_t eqPos = match.find_first_of('=', pos);
        if ((eqPos == String::npos) || (eqPos >= endPos)) {
            status = ER_FAIL;
            break;
        }
        ++eqPos;
        size_t begQuotePos = match.find_first_of('\'', eqPos);
        size_t endQuotePos = String::npos;
        if ((begQuotePos != String::npos) && (++begQuotePos < match.length())) {
            endQuotePos = match.find_first_of('\'', begQuotePos);
        }
        if (endQuotePos == String::npos) {
            status = ER_FAIL;
            break;
        }
        String key = match.substr(pos, eqPos - 1 - pos);
        String value = match.substr(begQuotePos, endQuotePos - begQuotePos);
        matchMap[key] = value;
        pos = endPos + 1;
    }
    return status;
}

}
