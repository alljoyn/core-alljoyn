/**
 * @file
 *
 * Unit test of the name validation checks
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

#include <assert.h>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include <alljoyn/Message.h>
#include <alljoyn/version.h>
#include "../src/BusUtil.h"

#include <alljoyn/Status.h>

using namespace qcc;
using namespace std;
using namespace ajn;

static const char* strings[] = {
    "foo",
    ":foo",
    ":foo.2",
    "/foo/bar",
    "/foo//bar",
    "/foo/bar/",
    "foo/bar/",
    "/",
    "foo/bar/",
    "foo.bar",
    ".foo.bar",
    "foo.bar.",
    "foo..bar",
    "_._._",
    "-.-.-",
    "8.8.8",
    "999",
    "_999",
    ":1.0",
    ":1.0.2.3.4",
    ":1.0.2.3..4",
    ":1.0.2.3.4.",
    ":.1.0"
};


void check(const char* str)
{

    if (IsLegalUniqueName(str)) {
        printf("\"%s\" is a unique name\n", str);
    } else {
        printf("\"%s\" is not a unique name\n", str);
    }
    if (IsLegalBusName(str)) {
        printf("\"%s\" is a bus name\n", str);
    } else {
        printf("\"%s\" is not a bus name\n", str);
    }
    if (IsLegalObjectPath(str)) {
        printf("\"%s\" is an object path\n", str);
    } else {
        printf("\"%s\" is not an object path\n", str);
    }
    if (IsLegalInterfaceName(str)) {
        printf("\"%s\" is an interface name\n", str);
    } else {
        printf("\"%s\" is not an interface name\n", str);
    }
    if (IsLegalErrorName(str)) {
        printf("\"%s\" is an error name\n", str);
    } else {
        printf("\"%s\" is not an error name\n", str);
    }
    if (IsLegalMemberName(str)) {
        printf("\"%s\" is a member name\n", str);
    } else {
        printf("\"%s\" is not a member name\n", str);
    }
}

void PadTo(char* buf, const char* str, size_t len, char pad)
{
    strcpy(buf, str);
    for (size_t i = strlen(buf); i < len; i++) {
        buf[i] = pad;
    }
    buf[len] = 0;
}

int main(int argc, char** argv)
{
    char buf[512];

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /*
     * Basic checks - should all pass
     */
    if (!IsLegalUniqueName(":1.0")) {
        printf("failed IsLegalUniqueName");
        return 1;
    }
    if (!IsLegalBusName("th_is.t9h-At")) {
        printf("failed IsLegalBusName");
        return 1;
    }
    if (!IsLegalObjectPath("/This/tha_t/99")) {
        printf("failed IsLegalObjectPath");
        return 1;
    }
    if (!IsLegalInterfaceName("THIS._that._1__")) {
        printf("failed IsLegalInterfaceName");
        return 1;
    }
    if (!IsLegalMemberName("this2Isa_member")) {
        printf("failed IsLegalMemberName");
        return 1;
    }
    /*
     * Maximum length checks - should all pass
     */
    PadTo(buf, ":1.0.", 255, '0');
    assert(strlen(buf) == 255);
    if (!IsLegalUniqueName(buf)) {
        printf("failed max IsLegalUniqueName");
        return 1;
    }
    PadTo(buf, "abc.def.hij.", 255, '-');
    if (!IsLegalBusName(buf)) {
        printf("failed max IsLegalBusName");
        return 1;
    }
    PadTo(buf, "abc.def.hij.", 255, '_');
    if (!IsLegalInterfaceName(buf)) {
        printf("failed max IsLegalInterfaceName");
        return 1;
    }
    PadTo(buf, "member", 255, '_');
    if (!IsLegalMemberName(buf)) {
        printf("failed max IsLegalMemberName");
        return 1;
    }
    /*
     * There is no maximum length for object paths
     */
    PadTo(buf, "/object/path/long/", 500, '_');
    if (!IsLegalObjectPath(buf)) {
        printf("failed long IsLegalObjectPath");
        return 1;
    }
    /*
     * Beyond maximum length checks - should all fail
     */
    PadTo(buf, ":1.0.", 256, '0');
    assert(strlen(buf) == 256);
    if (IsLegalUniqueName(buf)) {
        printf("failed too long IsLegalUniqueName");
        return 1;
    }
    PadTo(buf, "abc.def.hij.", 256, '-');
    if (IsLegalBusName(buf)) {
        printf("failed too long IsLegalBusName");
        return 1;
    }
    PadTo(buf, "abc.def.hij.", 256, '_');
    if (IsLegalInterfaceName(buf)) {
        printf("failed too long IsLegalInterfaceName");
        return 1;
    }
    PadTo(buf, "member", 256, '_');
    if (IsLegalMemberName(buf)) {
        printf("failed too long IsLegalMemberName");
        return 1;
    }


    for (size_t i = 0; i < ArraySize(strings); i++) {
        check(strings[i]);
    }
    return 0;
}
