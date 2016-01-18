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
#ifndef ABOUT_DATA_STORE_INTERFACE_H_
#define ABOUT_DATA_STORE_INTERFACE_H_

#include <stdio.h>
#include <iostream>
#include <alljoyn/AboutData.h>

#if defined(QCC_OS_GROUP_WINDOWS)
/* Disabling warning C 4100. Function doesnt use all passed in parameters */
#pragma warning(push)
#pragma warning(disable: 4100)
#endif


/**
 * The language tag specified is not supported
 */
#define ER_LANGUAGE_NOT_SUPPORTED       ((QStatus)0xb001)
/**
 * The request feature is not available or has not be implemented
 */
#define ER_FEATURE_NOT_AVAILABLE        ((QStatus)0xb002)
/**
 * The value requested is invalid
 */
#define ER_INVALID_VALUE                ((QStatus)0xb003)
/**
 * The maximum allowed size for an element has been exceeded.
 */
#define ER_MAX_SIZE_EXCEEDED            ((QStatus)0xb004)


/**
 * Structure to hold Filter enum
 */
struct DataPermission {
/**
 * Filter has three possible values ANNOUNCE, READ,WRITE
 * READ is for data that is marked as read
 * ANNOUNCE is for data that is marked as announce
 * WRITE is for data that is marked as write
 */
    typedef enum {
        ANNOUNCE,         //!< ANNOUNCE Property that has  ANNOUNCE  enabled
        READ,            //!< READ     Property that has READ  enabled
        WRITE,           //!< WRITE    Property that has  WRITE  enabled
    } Filter;
};

/**
 * class AboutDataStoreInterface - interface that handles remote config server requests
 * AboutDataStoreInterface store implementation
 */
class AboutDataStoreInterface : public ajn::AboutData {

  public:
    /**
     * AboutDataStoreInterface - constructor
     * @param factoryConfigFile
     * @param configFile
     */
    AboutDataStoreInterface(const char* factoryConfigFile, const char* configFile)
        : ajn::AboutData("en")
    {
        QCC_UNUSED(factoryConfigFile);
        QCC_UNUSED(configFile);
    }

    /**
     * FactoryReset
     */
    virtual void FactoryReset() = 0;


    /**
     * virtual Destructor
     */
    virtual ~AboutDataStoreInterface() { };

    /**
     * virtual method ReadAll
     * @param languageTag
     * @param filter
     * @param all
     * @return QStatus
     */
    virtual QStatus ReadAll(const char* languageTag, DataPermission::Filter filter, ajn::MsgArg& all) = 0;

    /**
     * virtual method Update
     * @param name
     * @param languageTag
     * @param value
     * @return QStatus
     */
    virtual QStatus Update(const char* name, const char* languageTag, const ajn::MsgArg* value) = 0;

    /**
     * virtual method Delete
     * @param name
     * @param languageTag
     * @return QStatus
     */
    virtual QStatus Delete(const char* name, const char* languageTag) = 0;

  private:

};

#endif /* ABOUT_DATA_STORE_INTERFACE_H_ */
