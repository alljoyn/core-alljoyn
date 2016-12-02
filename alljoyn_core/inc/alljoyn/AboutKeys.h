/**
 * @file
 * This contains the AboutData class responsible for holding the org.alljoyn.About
 * interface data fields.
 */
/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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