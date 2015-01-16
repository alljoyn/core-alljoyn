/**
 * @file
 * Common file for holding global variables.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2014-2015, AllSeen Alliance. All rights reserved.
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
#include <qcc/CommonGlobals.h>
#include <qcc/String.h>
#include <qcc/Logger.h>
#include <qcc/Util.h>

#define QCC_MODULE "STATICGLOBALS"

#ifdef QCC_OS_GROUP_WINDOWS
#include <qcc/windows/utility.h>
#endif
#ifdef CRYPTO_CNG
#include <qcc/CngCache.h>
#endif
namespace qcc {
/** Assign enough memory to store all static/global values */
static uint64_t commonDummy[RequiredArrayLength(sizeof(StaticGlobals), uint64_t)];

/** Assign commonGlobals to be a reference to the allocated memory */
StaticGlobals& commonGlobals = ((StaticGlobals &)commonDummy);

Event& Event::alwaysSet = (Event &)commonGlobals.alwaysSet;
Event& Event::neverSet = (Event &)commonGlobals.neverSet;

StaticGlobals::StaticGlobals() :
    alwaysSet(0, 0),
    neverSet(Event::WAIT_FOREVER, 0)
{
}

}

using namespace qcc;
static int alljoynCounter = 0;
bool StaticGlobalsInit::cleanedup = false;
/** Note: StaticGlobalsInit struct is defined in platform.h */
StaticGlobalsInit::StaticGlobalsInit()
{
    if (alljoynCounter++ == 0) {
        /* Initialize globals in common */
        new (&commonGlobals)StaticGlobals(); // placement new operator
    }
}

StaticGlobalsInit::~StaticGlobalsInit()
{
    if (--alljoynCounter == 0 && !cleanedup) {
        /* Uninitialize globals in common */
        commonGlobals.~StaticGlobals(); // explicitly call the destructor
        cleanedup = true;
    }
}

void StaticGlobalsInit::Cleanup()
{
    if (!cleanedup) {
        /* Uninitialize globals in common */
        commonGlobals.~StaticGlobals(); // explicitly call the destructor
        cleanedup = true;
    }
    LoggerInit::Cleanup();
    StringInit::Cleanup();
#ifdef QCC_OS_GROUP_WINDOWS
    WinsockInit::Cleanup();
#endif
#ifdef CRYPTO_CNG
    CngCacheInit::Cleanup();
#endif
}
