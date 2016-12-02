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

#include <alljoyn/about/PropertyStoreProperty.h>

namespace ajn {
namespace services {

PropertyStoreProperty::PropertyStoreProperty(qcc::String const& keyName) :
    m_Keyname(keyName), m_Public(true), m_Writable(false), m_Announcable(true), m_Language("")
{
}

PropertyStoreProperty::PropertyStoreProperty(qcc::String const& keyName, ajn::MsgArg const& value) :
    m_Keyname(keyName), m_Value(value), m_Public(true), m_Writable(false), m_Announcable(true),
    m_Language("")
{
}

PropertyStoreProperty::PropertyStoreProperty(qcc::String const& keyName, ajn::MsgArg const& value,
                                             bool isPublic, bool isWritable, bool isAnnouncable) : m_Keyname(keyName),
    m_Value(value), m_Public(isPublic), m_Writable(isWritable),
    m_Announcable(isAnnouncable), m_Language("")
{
}

PropertyStoreProperty::PropertyStoreProperty(qcc::String const& keyName, ajn::MsgArg const& value, qcc::String const& language,
                                             bool isPublic, bool isWritable, bool isAnnouncable) : m_Keyname(keyName),
    m_Value(value), m_Public(isPublic), m_Writable(isWritable),
    m_Announcable(isAnnouncable), m_Language(language)
{
}

PropertyStoreProperty::~PropertyStoreProperty()
{

}

void PropertyStoreProperty::setFlags(bool isPublic, bool isWritable, bool isAnnouncable)
{
    m_Public = isPublic;
    m_Writable = isWritable;
    m_Announcable = isAnnouncable;
}

void PropertyStoreProperty::setLanguage(qcc::String const& language)
{
    m_Language = language;
}

const qcc::String& PropertyStoreProperty::getPropertyName() const {
    return m_Keyname;
}

const ajn::MsgArg& PropertyStoreProperty::getPropertyValue() const {
    return m_Value;
}

const qcc::String& PropertyStoreProperty::getLanguage() const {
    return m_Language;
}

bool PropertyStoreProperty::getIsPublic() const {
    return m_Public;
}

bool PropertyStoreProperty::getIsWritable() const {
    return m_Writable;
}

bool PropertyStoreProperty::getIsAnnouncable() const {
    return m_Announcable;
}

void PropertyStoreProperty::setIsPublic(bool value) {
    m_Public = value;
}

void PropertyStoreProperty::setIsWritable(bool value) {
    m_Writable = value;
}

void PropertyStoreProperty::setIsAnnouncable(bool value) {
    m_Announcable = value;
}

} /* namespace services */
} /* namespace ajn */