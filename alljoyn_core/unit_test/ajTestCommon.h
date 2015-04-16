/******************************************************************************
 *
 *
 * Copyright AllSeen Alliance. All rights reserved.
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
#ifndef AJTESTCOMMON_H
#define AJTESTCOMMON_H

#include <qcc/String.h>
#include <alljoyn/BusAttachment.h>

/*
 * this header file contains a functions that can be used to replace common
 * actions in the test code.
 */
namespace ajn {

/**
 * Obtain the default connection arg for the OS the test is run on.
 * If running on on windows this should be "tcp:addr=127.0.0.1,port=9956"
 * If running on a unix variant this should be "unix:abstract=alljoyn"
 *
 * The environment variable BUS_ADDRESS is specified it will be used in place
 * of the default address
 *
 * @return a qcc::String containing the default connection arg
 */
qcc::String getConnectArg(const char* envvar = "BUS_ADDRESS");

/**
 * Generate a globally unique name for use in advertising.
 *
 * Advertised names should be unique to avoid multiple running instances
 * of the test suite from interferring with each other.
 */
qcc::String genUniqueName(const BusAttachment& bus);

/**
 * Get the prefix of the uniqueNames used in advertising
 *
 * Advertised names should be unique to avoid multiple running instances
 * of the test suite from interferring with each other.
 */
qcc::String getUniqueNamePrefix(const BusAttachment& bus);

/**
 * Granularity of GetTimestamp64().
 *
 * GetTimestamp64() uses GetTickCount64 as source of time on Windows.
 * GetTickCount64() typically has a 10-16 milliseconds granularity, so
 * the result of on GetTimestamp64() on Windows can be up to 15 milliseconds
 * smaller than expected at a given time.
 */
#if defined(QCC_OS_GROUP_WINDOWS)
    #define TIMESTAMP_GRANULARITY 0
#else
    #define TIMESTAMP_GRANULARITY 0
#endif

}

/*
 * gtest printers
 */
void PrintTo(const QStatus& status, ::std::ostream* os);

#endif //AJTESTCOMMON_H
