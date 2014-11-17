/**
 * @file
 * Common file for holding global variables.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#define QCC_MODULE "COMMONGLOBALS"

namespace qcc {
/** Assign enough memory to store all static/global values */
static uint64_t commonDummy[sizeof(CommonGlobals) / 4];

/** Assign commonGlobals to be a reference to the allocated memory */
CommonGlobals& commonGlobals = ((CommonGlobals &)commonDummy);

Event& Event::alwaysSet = (Event &)commonGlobals.alwaysSet;
Event& Event::neverSet = (Event &)commonGlobals.neverSet;

CommonGlobals::CommonGlobals() :
    alwaysSet(0, 0),
    neverSet(Event::WAIT_FOREVER, 0)
{
}

}

using namespace qcc;
static int alljoynCounter = 0;
/** Note: AllJoynGlobalsInit struct is defined in platform.h */
AllJoynGlobalsInit::AllJoynGlobalsInit() {
    if (alljoynCounter++ == 0) {
        /* Initialize globals in common */
        new (&commonGlobals)CommonGlobals(); // placement new operator
    }
}

AllJoynGlobalsInit::~AllJoynGlobalsInit() {
    if (--alljoynCounter == 0) {
        /* Uninitialize globals in common */
        commonGlobals.~CommonGlobals(); // explicitly call the destructor
    }
}
