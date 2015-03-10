/**
 * @file
 * User implemented property store responsible for holding the AboutService properties
 * @see AboutPropertyStoreImpl.h
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
#ifndef _PROPERTYSTORE_H
#define _PROPERTYSTORE_H

#include <alljoyn/Status.h>
#include <alljoyn/MsgArg.h>

namespace ajn {
namespace services {
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
 * PropertyStore is implemented by the application , it is responsible to store
 * the properties of the AboutServie and ConfigService.
 */
class PropertyStore {
  public:

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

    /**
     * Calls Reset() implemented only for  ConfigService
     * @return status
     */
    virtual QStatus Reset() {
        return ER_NOT_IMPLEMENTED;
    }

    /**
     *
     * @param[in] languageTag is the language to use for the action can be NULL meaning default.
     * @param[in] filter describe which properties to read.
     * @param[out] all reference to MsgArg
     * @return status
     */
    virtual QStatus ReadAll(const char* languageTag, Filter filter, ajn::MsgArg& all) = 0;
    /**
     *
     * @param[in] name of the property
     * @param[in] languageTag is the language to use for the action can be NULL meaning default.
     * @param[in] value is a pointer to the data to change.
     * @return status
     */
    virtual QStatus Update(const char* name, const char* languageTag, const ajn::MsgArg* value) {
        return ER_NOT_IMPLEMENTED;
    }
    /**
     *
     * @param[in] name of the property
     * @param[in] languageTag is the language to use for the action can't be NULL.
     * @return[in] status
     */
    virtual QStatus Delete(const char* name, const char* languageTag) {
        return ER_NOT_IMPLEMENTED;
    }
    /**
     *  Desctructor of PropertyStore;
     */
    virtual ~PropertyStore() = 0;
};
inline PropertyStore::~PropertyStore() {
}

}
}

#endif /* _PROPERTYSTORE_H */
