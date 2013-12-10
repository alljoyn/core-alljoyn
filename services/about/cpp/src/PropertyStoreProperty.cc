/******************************************************************************
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
