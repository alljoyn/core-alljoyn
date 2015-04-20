/**
 * @file
 * This file provides access to AllJoyn library version and build information.
 */

/******************************************************************************
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
#ifndef _ALLJOYN_VERSION_H
#define _ALLJOYN_VERSION_H

#include <qcc/platform.h>

/** Macro to compute the version number into a single value. */
#define GenerateNumericVersionValue(_year, _month, _feature, _bugfix) (((_year) << 24) | ((_month) << 16) | ((_feature) << 8) | (_bugfix))

/** Macro to extract the year from unified version value. */
#define GetVersionYear(_ver)    (((_ver) >> 24) & 0xff)

/** Macro to extract the month from unified version value. */
#define GetVersionMonth(_ver)   (((_ver) >> 16) & 0xff)

/** Macro to extract the feature from unified version value. */
#define GetVersionFeature(_ver) (((_ver) >>  8) & 0xff)

/** Macro to extract the bugfix from unified version value. */
#define GetVersionBugfix(_ver)  (((_ver) >>  0) & 0xff)

/**
 * Macro to compute the version number into a single value.
 *
 * @deprecated
 *
 * @see GenerateNumericVersionValue()
 */
#define GenerateVersionValue(_arch, _api, _rel) (((_arch) << 24) | ((_api) << 16) | (_rel))

/**
 * Macro to extract the architecture level from unified version value.
 *
 * @deprecated
 *
 * @see GetVersionYear()
 */
#define GetVersionArch(_ver) (((_ver) >> 24) & 0xff)

/**
 * Macro to extract the API level from unified version value.
 *
 * @deprecated
 *
 * @see GetVersionMonth()
 */
#define GetVersionAPILevel(_ver) (((_ver) >> 16) & 0xff)

/**
 * Macro to extract the release from unified version value.
 *
 * @deprecated
 *
 * @see GetVersionFeature()
 * @see GetVersionBugfix()
 */
#define GetVersionRelease(_ver) ((_ver) & 0xffff)

namespace ajn {
const char* AJ_CALL GetVersion();        /**< Gives the version of AllJoyn Library */
const char* AJ_CALL GetBuildInfo();      /**< Gives build information of AllJoyn Library */
uint32_t AJ_CALL GetNumericVersion();  /**< Gives the version of AllJoyn Library as a single number */
}

#endif
