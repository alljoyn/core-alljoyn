/******************************************************************************
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
#include "PropertyStoreImpl.h"

using namespace ajn;
using namespace services;

PropertyStoreImpl::Property::Property(qcc::String const& aKeyName) :
    keyname(aKeyName), bPublic(false), bWritable(false), bAnnouncable(false)
{
}

PropertyStoreImpl::Property::Property(qcc::String const& aKeyName, ajn::MsgArg const& aValue) :
    keyname(aKeyName), value(aValue), bPublic(false), bWritable(false), bAnnouncable(false)
{
}

PropertyStoreImpl::Property::Property(qcc::String const& aKeyName, ajn::MsgArg const& aValue,
                                      bool isPublic, bool isWritable, bool isAnnouncable) :
    keyname(aKeyName), value(aValue), bPublic(isPublic), bWritable(isWritable), bAnnouncable(isAnnouncable)
{
}

qcc::String& PropertyStoreImpl::Property::GetkeyName()
{
    return keyname;
}

const ajn::MsgArg& PropertyStoreImpl::Property::GetkeyValue()
{
    return value;
}

qcc::String& PropertyStoreImpl::Property::GetLanguage()
{
    return language;
}

void PropertyStoreImpl::Property::SetKeyName(qcc::String const& aKeyname)
{
    keyname = aKeyname;
}

void PropertyStoreImpl::Property::SetKeyValue(ajn::MsgArg const& aValue)
{
    value = aValue;
}

void PropertyStoreImpl::Property::SetLanguage(qcc::String const& aLanguage)
{
    language = aLanguage;
}

void PropertyStoreImpl::Property::SetFlags(bool isPublic, bool isWritable, bool isAnnouncable)
{
    bPublic = isPublic;
    bWritable = isWritable;
    bAnnouncable = isAnnouncable;
}

bool PropertyStoreImpl::Property::GetIsPublic()
{
    return bPublic;
}

bool PropertyStoreImpl::Property::GetIsWritable()
{
    return bWritable;
}

bool PropertyStoreImpl::Property::GetIsAnnouncable()
{
    return bAnnouncable;
}

void PropertyStoreImpl::Property::SetIsPublic(bool value)
{
    bPublic = value;
}

void PropertyStoreImpl::Property::SetIsWritable(bool value)
{
    bWritable = value;
}

void PropertyStoreImpl::Property::SetIsAnnouncable(bool value)
{
    bAnnouncable = value;
}

PropertyStoreImpl::PropertyStoreImpl(std::multimap<qcc::String, Property>& data)
{
    internalMultimap = data;
}

PropertyStoreImpl::~PropertyStoreImpl()
{
}

QStatus PropertyStoreImpl::ReadAll(const char* languageTag, Filter filter, ajn::MsgArg& all)
{
    if (filter == ANNOUNCE) {

        std::multimap<qcc::String, Property>::iterator search = internalMultimap.find("DefaultLanguage");

        qcc::String defaultLanguage;
        if (search != internalMultimap.end()) {
            char*temp;
            search->second.GetkeyValue().Get("s", &temp);
            defaultLanguage.assign(temp);
        }

        int announceArgCount = 0;
        for (std::multimap<qcc::String, Property>::const_iterator it = internalMultimap.begin(); it != internalMultimap.end();
             ++it) {
            qcc::String key = it->first;
            Property property = it->second;
            if (!property.GetIsAnnouncable()) {
                continue;
            }
            // check that it is from the defaultLanguage or empty.
            if (!(property.GetLanguage().empty() || property.GetLanguage().compare(defaultLanguage) == 0)) {
                continue;
            }
            announceArgCount++;
        }
        MsgArg* argsAnnounceData = new MsgArg[announceArgCount];
        announceArgCount = 0;
        for (std::multimap<qcc::String, Property>::const_iterator it = internalMultimap.begin(); it != internalMultimap.end();
             ++it) {
            qcc::String key = it->first;
            Property property = it->second;
            if (!property.GetIsAnnouncable()) {
                continue;
            }
            // check that it is from the defaultLanguage or empty.
            if (!(property.GetLanguage().empty() || property.GetLanguage().compare(defaultLanguage) == 0)) {
                continue;
            }

            //QStatus status =
            argsAnnounceData[announceArgCount].Set("{sv}", property.GetkeyName().c_str(),
                                                   new MsgArg(property.GetkeyValue()));
            argsAnnounceData[announceArgCount].SetOwnershipFlags(MsgArg::OwnsArgs, true);
            announceArgCount++;
        }
        //QStatus status =
        all.Set("a{sv}", announceArgCount, argsAnnounceData);
        all.SetOwnershipFlags(MsgArg::OwnsArgs, true);
        //printf("ReadAll ANNOUNCE status %d\n",status);

    } else if (filter == READ) {

        if (languageTag != NULL && languageTag[0] != 0) {                         // check that the language is in the supported languages;
            std::multimap<qcc::String, Property>::iterator it = internalMultimap.find("SupportedLanguages");
            if (it == internalMultimap.end()) {
                return ER_LANGUAGE_NOT_SUPPORTED;
            }

            const MsgArg*stringArray;
            size_t fieldListNumElements;
            it->second.GetkeyValue().Get("as", &fieldListNumElements, &stringArray);
            bool found = false;
            for (size_t i = 0; i < fieldListNumElements; i++) {
                char* tempString;
                stringArray[i].Get("s", &tempString);
                if (strcmp(tempString, languageTag) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return ER_LANGUAGE_NOT_SUPPORTED;
            }
        } else {
            std::multimap<qcc::String, Property>::iterator it = internalMultimap.find("DefaultLanguage");
            if (it == internalMultimap.end()) {
                return ER_LANGUAGE_NOT_SUPPORTED;
            }
            it->second.GetkeyValue().Get("s", &languageTag);
        }

        int readArgCount = 0;
        for (std::multimap<qcc::String, Property>::const_iterator it = internalMultimap.begin(); it != internalMultimap.end();
             ++it) {
            qcc::String key = it->first;
            Property property = it->second;
            if (!property.GetIsPublic()) {
                continue;
            }
            // check that it is from the defaultLanguage or empty.
            if (!(property.GetLanguage().empty() || property.GetLanguage().compare(languageTag) == 0)) {
                continue;
            }
            readArgCount++;
        }
        MsgArg*argsReadData = new MsgArg[readArgCount];
        readArgCount = 0;
        for (std::multimap<qcc::String, Property>::const_iterator it = internalMultimap.begin(); it != internalMultimap.end();
             ++it) {
            qcc::String key = it->first;
            Property property = it->second;
            if (!property.GetIsPublic()) {
                continue;
            }
            // check that it is from the defaultLanguage or empty.
            if (!(property.GetLanguage().empty() || property.GetLanguage().compare(languageTag) == 0)) {
                continue;
            }

            //QStatus status =
            argsReadData[readArgCount].Set("{sv}", property.GetkeyName().c_str(),
                                           new MsgArg(property.GetkeyValue()));
            argsReadData[readArgCount].SetOwnershipFlags(MsgArg::OwnsArgs, true);
            readArgCount++;
        }
        //QStatus status =
        all.Set("a{sv}", readArgCount, argsReadData);
        all.SetOwnershipFlags(MsgArg::OwnsArgs, true);
        //printf("ReadAll READ status %d\n",status);
    } else {
        return ER_NOT_IMPLEMENTED;
    }
    return ER_OK;
    //printf("all.ToString() : %s\n\n", all.ToString().c_str());
}

QStatus PropertyStoreImpl::Update(const char* name, const char* languageTag, const ajn::MsgArg* value)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus PropertyStoreImpl::Delete(const char* name, const char* languageTag)
{
    return ER_NOT_IMPLEMENTED;
}
