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
#ifndef PROPERTYSTOREIMPL_H_
#define PROPERTYSTOREIMPL_H_

#include <stdio.h>
#include <iostream>

#include <alljoyn/about/AboutServiceApi.h>
#include <alljoyn/about/PropertyStore.h>

class PropertyStoreImpl : public ajn::services::PropertyStore {

  public:
    class Property {
      public:
        Property(qcc::String const& aKeyName);

        Property(qcc::String const& aKeyName, ajn::MsgArg const& aValue);

        Property(qcc::String const& aKeyName, ajn::MsgArg const& aValue, bool isPublic, bool isWritable, bool isAnnouncable);

        qcc::String& GetkeyName();

        const ajn::MsgArg& GetkeyValue();

        qcc::String& GetLanguage();

        void SetKeyName(qcc::String const& aKeyname);

        void SetKeyValue(ajn::MsgArg const& aValue);

        void SetLanguage(qcc::String const& aLanguage);

        void SetFlags(bool isPublic, bool isWritable, bool isAnnouncable);

        bool GetIsPublic();

        bool GetIsWritable();

        bool GetIsAnnouncable();

        void SetIsPublic(bool value);

        void SetIsWritable(bool value);

        void SetIsAnnouncable(bool value);

      private:
        qcc::String keyname;
        ajn::MsgArg value;
        bool bPublic;
        bool bWritable;
        bool bAnnouncable;
        qcc::String language;
    };

    PropertyStoreImpl(std::multimap<qcc::String, Property>& data);

    virtual ~PropertyStoreImpl();

    virtual QStatus ReadAll(const char* languageTag, Filter filter, ajn::MsgArg& all);

    virtual QStatus Update(const char* name, const char* languageTag, const ajn::MsgArg* value);

    virtual QStatus Delete(const char* name, const char* languageTag);

  private:

    std::multimap<qcc::String, Property> internalMultimap;

};

#endif /* PROPERTYSTOREIMPL_H_ */
