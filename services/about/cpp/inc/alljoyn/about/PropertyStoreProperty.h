/**
 * @file
 * property of the PropertyStoreImpl class
 * @see PropertyStoreImpl.h
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

#ifndef PROPERTYSTOREPROP_H_
#define PROPERTYSTOREPROP_H_

#include <qcc/String.h>
#include <alljoyn/MsgArg.h>

namespace ajn {
namespace services {

/**
 * Property of Property Store Impl class
 */
class PropertyStoreProperty {

  public:

    /**
     * Constructor for PropertyStoreProperty
     * @param keyName
     */
    PropertyStoreProperty(qcc::String const& keyName);

    /**
     * Constructor for PropertyStoreProperty
     * @param keyName
     * @param value
     */
    PropertyStoreProperty(qcc::String const& keyName, ajn::MsgArg const& value);

    /**
     * Constructor for PropertyStoreProperty
     * @param keyName
     * @param value
     * @param isPublic
     * @param isWritable
     * @param isAnnouncable
     */
    PropertyStoreProperty(qcc::String const& keyName, ajn::MsgArg const& value,
                          bool isPublic, bool isWritable, bool isAnnouncable);

    /**
     * Constructor for PropertyStoreProperty
     * @param keyName
     * @param value
     * @param language
     * @param isPublic
     * @param isWritable
     * @param isAnnouncable
     */
    PropertyStoreProperty(qcc::String const& keyName, ajn::MsgArg const& value, qcc::String const& language,
                          bool isPublic, bool isWritable, bool isAnnouncable);


    /**
     * Destructor for PropertyStoreProperty
     */
    virtual ~PropertyStoreProperty();

    /**
     * Set the flags for the property
     * @param isPublic
     * @param isWritable
     * @param isAnnouncable
     */
    void setFlags(bool isPublic, bool isWritable, bool isAnnouncable);

    /**
     * Set the language of the property
     * @param language
     */
    void setLanguage(qcc::String const& language);

    /**
     * Get the KeyName of the property
     * @return keyname
     */
    const qcc::String& getPropertyName() const;

    /**
     * Get the Value of the Property
     * @return value
     */
    const ajn::MsgArg& getPropertyValue() const;

    /**
     * Get the Language of the Property
     * @return Language
     */
    const qcc::String& getLanguage() const;

    /**
     * Get the isPublic boolean of the property
     * @return isPublic
     */
    bool getIsPublic() const;

    /**
     * Get the isWritable boolean of the Property
     * @return isWritable
     */
    bool getIsWritable() const;

    /**
     * Get the isAnnounceable boolean of the Property
     * @return isAnnounceable
     */
    bool getIsAnnouncable() const;

    /**
     * Set the isPublic boolean of the Property
     * @param value
     */
    void setIsPublic(bool value);

    /**
     * set the IsWritable boolean of the Property
     * @param value
     */
    void setIsWritable(bool value);

    /**
     * set the isAnnounce of the Property
     * @param value
     */
    void setIsAnnouncable(bool value);

  private:

    /**
     * private member keyName
     */
    qcc::String m_Keyname;

    /**
     * private member value
     */
    ajn::MsgArg m_Value;

    /**
     * private member isPublic
     */
    bool m_Public;

    /**
     * private member isWritable
     */
    bool m_Writable;

    /**
     * private member isAnnounceable
     */
    bool m_Announcable;

    /**
     * private member language
     */
    qcc::String m_Language;
};
} /* namespace services */
} /* namespace ajn */
#endif /* PROPERTYSTOREPROP */
