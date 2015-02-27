/**
 * @file
 * This contains the AboutData class responsible for holding the org.alljoyn.About
 * interface data fields.
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
#ifndef _ALLJOYN_ABOUTKEYS_H
#define _ALLJOYN_ABOUTKEYS_H

#include <alljoyn/AboutDataListener.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/Status.h>

#include <qcc/String.h>

namespace ajn {

/**
 * AboutKeys
 */
class AboutKeys {
  public:
    /**
     * @anchor AboutFields
     * @name Known AboutFields
     *
     * The known fields in the About interface
     * TODO put in a table listing the properties for all of the about fields
     */
    // @{
    static const char* APP_ID;           ///< The globally unique id for the application
    static const char* DEFAULT_LANGUAGE; ///< The default language supported by the device. IETF language tags specified by RFC 5646.
    static const char* DEVICE_NAME; ///< The name of the device
    static const char* DEVICE_ID; ///< A unique string with a value generated using platform specific means
    static const char* APP_NAME; ///< The application name assigned by the manufacture
    static const char* MANUFACTURER; ///< The manufacture's name
    static const char* MODEL_NUMBER; ///< The application model number
    static const char* SUPPORTED_LANGUAGES; ///< List of supported languages
    static const char* DESCRIPTION; ///< Detailed description provided by the application
    static const char* DATE_OF_MANUFACTURE; ///< The date of manufacture using format YYYY-MM-DD
    static const char* SOFTWARE_VERSION; ///< The software version for the OEM software
    static const char* AJ_SOFTWARE_VERSION; ///< The current version of the AllJoyn SDK utilized by the application
    static const char* HARDWARE_VERSION; ///< The device hardware version
    static const char* SUPPORT_URL; ///< The support URL provided by the OEM or software developer
    // @}
};
}
#endif //_ALLJOYN_ABOUTKEYS_H
