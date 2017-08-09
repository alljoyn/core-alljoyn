/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/
#ifndef _ALLJOYN_ABOUTDATAINTERNAL_H
#define _ALLJOYN_ABOUTDATAINTERNAL_H
#include <map>
#include <set>

#include <alljoyn/AboutData.h>
#include <algorithm>
#include <cctype>

namespace ajn {

/**
 * Class that provides translations of text using a lookup table where
 * each translated string is stored as a MsgArg as currently required
 * by AboutData.
 */
class MsgArgTableTranslator : public LookupTableTranslator {
  public:
    /**
     * @see LookupTableTranslator::NumFields
     */
    virtual size_t NumFields()
    {
        return localizedStore.size();
    }

    /**
     * @see LookupTableTranslator::GetFieldId
     */
    virtual const char* GetFieldId(size_t index)
    {
        localizedStoreIterator it = localizedStore.begin();
        for (size_t count = 0; (count < index) && (it != localizedStore.end()); it++, count++)
            ; //empty for loop to advance the iterator to the element indicated by the index
        if (it == localizedStore.end()) {
            return nullptr;
        }
        return it->first.c_str();
    }

    /**
     * @see Translator::Translate
     */
    virtual const char* Translate(const char* sourceLanguage, const char* targetLanguage, const char* sourceText)
    {
        QCC_UNUSED(sourceLanguage);
        char* result;
        QStatus status = localizedStore[sourceText][targetLanguage].Get("s", &result);
        return (status == ER_OK) ? result : nullptr;
    }

    /**
     * @see Translator::TranslateToMsgArg
     */
    virtual QStatus TranslateToMsgArg(const char* sourceLanguage,
                                      const char* targetLanguage, const char* sourceText, MsgArg*& value)
    {
        QCC_UNUSED(sourceLanguage);
        value = &localizedStore[sourceText][targetLanguage];
        return (value->typeId == ALLJOYN_INVALID) ? ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE : ER_OK;
    }

    /**
     * @see Translator::AddMsgArgTranslation
     */
    virtual QStatus AddMsgArgTranslation(const char* id, const MsgArg* value, const char* language)
    {
        if (supportedLanguages.find(language) == supportedLanguages.end()) {
            supportedLanguages.insert(language);
        }
        localizedStore[id][language] = *value;
        return ER_OK;
    }

  protected:
    /**
     * Local member variable mapping a field id to a set of translations
     * in various languages.
     */
    std::map<qcc::String, std::map<qcc::String, MsgArg, CaseInsensitiveCompare> > localizedStore;

    /**
     * typedef localized store iterator
     */
    typedef std::map<qcc::String, std::map<qcc::String, MsgArg, CaseInsensitiveCompare> >::iterator localizedStoreIterator;
};

/**
 * Class used to hold internal values for the AboutData class
 */
class AboutData::Internal {
    friend class AboutData;
  public:
    AboutData::Internal& operator=(const AboutData::Internal& other) {
        aboutFields = other.aboutFields;
        propertyStore = other.propertyStore;
        defaultTranslator = other.defaultTranslator;
        translator = other.translator;
        keyLanguage = other.keyLanguage;
        return *this;
    }

  private:
    /**
     * Check if the given character is a hexadecimal digit.
     *
     * @param c The character to check.
     * @return
     *  - true if the character is in one of the ranges: 0..9; a..f; A..F.
     *  - false otherwise.
     */
    bool isHexChar(char c) const;

    /**
     * Set field based on MsgArg.
     *
     * @param[in] name the name of the field to set
     * @param[in] value a MsgArg that contains the value that is set for the field
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is nullptr, the default language will be used.  Only
     *            used for fields that are marked as localizable.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED if language tag was not specified
     *                                             and the default language is also
     *                                             not found.
     */
    QStatus SetField(const char* name, MsgArg value, const char* language = nullptr);

    /**
     * Set field based on string value (char* version).
     *
     * @param[in] name the name of the field to set
     * @param[in] value the value to be set
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is nullptr, the default language will be used.  Only
     *            used for fields that are marked as localizable.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED if language tag was not specified
     *                                             and the default language is also
     *                                             not found.
     */
    QStatus SetField(const char* fieldName, const char* value, const char* language = nullptr);

    /**
     * Set field based on string value (qcc::String version).
     *
     * @param[in] name the name of the field to set
     * @param[in] value the value to be set
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is empty, the default language will be used.  Only
     *            used for fields that are marked as localizable.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED if language tag was not specified
     *                                             and the default language is also
     *                                             not found.
     */
    QStatus SetField(const qcc::String& fieldName, const qcc::String& value, const qcc::String& language = "");

    /**
     * Get field into a MsgArg.
     *
     * @param[in] name the name of the field to get
     * @param[out] value MsgArg holding a variant value that represents the field
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is nullptr, the field for the default language will be
     *            returned.
     *
     * @return ER_OK on success
     */
    QStatus GetField(const char* name, MsgArg*& value, const char* language = nullptr);

    /**
     * Get field into a string (char** version)
     *
     * @param[in] name the name of the field to get
     * @param[out] value the retrieved value
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is nullptr, the field for the default language will be
     *            returned.
     *
     * @return ER_OK on success
     */
    QStatus GetField(const char* fieldName, char** value, const char* language = nullptr);

    /**
     * Get field into a string (qcc::String version)
     *
     * @param[in] name the name of the field to get
     * @param[out] value the retrieved value
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is nullptr, the field for the default language will be
     *            returned.
     *
     * @return ER_OK on success
     */
    QStatus GetField(const qcc::String& fieldName, qcc::String& value, const qcc::String& language = "");

    /**
     * Set a supported language.
     *
     * This is a string representing a single language. The language is
     * specified using IETF language tags specified by the RFC 5646.
     *
     * If the language tag has already been added, ER_OK will be returned with no
     * additional changes being made.
     *
     * @param[in] language the IETF language tag
     *
     * @return ER_OK on success
     */
    QStatus SetSupportedLanguage(const char* language);

    /**
     * Fill in the AboutData fields using a MsgArg
     *
     * The MsgArg must contain a dictionary of type a{sv} The expected use of this
     * class is to fill in the AboutData using a MsgArg obtain from the Announce
     * signal or the GetAboutData method from org.alljoyn.about interface.
     *
     * @param arg MsgArg containing AboutData dictionary.
     * @param language the language for the MsgArg AboutData.
     *                 If nullptr the default language will be used
     *
     * @return ER_OK on success
     */
    QStatus CreateFromMsgArg(const MsgArg& arg, const char* language);

    /**
     * Is the given field name required to make an About announcement
     *
     * @param[in] fieldName the name of the field
     *
     * @return
     *  - true if the field is required to make an About announcement
     *  - false otherwise.  If the fieldName is unknown, false will be returned
     */
    bool IsFieldRequired(const char* fieldName) const;

    /**
     * Is the given field part of the announce signal
     *
     * @param[in] fieldName the name of the field
     *
     * @return
     *  - true if the field is part of the announce signal
     *  - false otherwise.  If the fieldName is unknown, false will be returned
     */
    bool IsFieldAnnounced(const char* fieldName) const;

    /**
     * Is the given field a localized field.
     *
     * Localized fields should be provided for every supported language
     *
     * @param[in] fieldName the name of the field
     *
     * @return
     *  - true if the field is a localizable value
     *  - false otherwise.  If the fieldName is unknown, false will be returned.
     */
    bool IsFieldLocalized(const char* fieldName) const;

    /**
     * Get the signature for the given field.
     *
     * @param[in] fieldName the name of the field
     *
     * @return
     *  - the signature of the field
     *  - nullptr is field is unknown
     */
    const char* GetFieldSignature(const char* fieldName) const;

    /**
     * Set the AppId for the AboutData
     *
     * AppId Should be a 128-bit UUID as specified in by RFC 4122.
     *
     * Passing in non-128-bit byte arrays will still Set the AppId but the
     * SetAppId member function will always return
     * ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE and the application will fail
     * certification and compliance testing.
     *
     * AppId IS required
     * AppId IS part of the Announce signal
     * AppId CAN NOT be localized for other languages
     *
     * @param[in] appId the a globally unique array of bytes used as an ID for the application
     * @param[in] num   the number of bites in the appId array
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE if the AppId is not a 128-bits (16 bytes)
     */
    QStatus SetAppId(const uint8_t* appId, const size_t num);

    /**
     * Set the AppId for the AboutData using a string.
     *
     * The string must be either a 32-character hex digit string
     * (i.e. 4a354637564945188a48323c158bc02d).
     * or a UUID string as specified in RFC 4122
     * (i.e. 4a354637-5649-4518-8a48-323c158bc02d)
     * AppId should be a 128-bit UUID as specified in by RFC 4122
     *
     * Unlike SetAppId(const uint8_t*, const size_t) this member function will
     * only set the AppId if the string is a 32-character hex digit string or a
     * UUID as specified by RFC 4122.
     *
     * AppId IS required
     * AppId IS part of the Announce signal
     * AppId CAN NOT be localized for other languages
     *
     * @see #SetAppId(const uint8_t*, const size_t)
     *
     * @param[in] appId String representing a globally unique array of bytes
     *                  used as an ID for the application.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE if the AppId is not a 128-bits (16 bytes)
     *  - #ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE if unable to parse the appId string.
     */
    QStatus SetAppId(const char* appId);

    /**
     * The AboutData has all of the required fields
     *
     * If a language field is given this will return if all required fields are
     * listed for the given language.
     *
     * If no language is given, the default language will be checked
     *
     * @param[in] language IETF language tags specified by RFC 5646
     *
     * @return true if all required field are listed for the given language
     */
    bool IsValid(const char* language = nullptr);

    /**
     * Get the DefaultLanguage from the AboutData
     *
     * @param[out] defaultLanguage a pointer to the default language tag
     *
     * @return ER_OK on success
     */
    QStatus GetDefaultLanguage(char** defaultLanguage);

    /**
     * A std::map that maps the field name to its FieldDetails.
     */
    std::map<qcc::String, FieldDetails> aboutFields;

    /**
     * property store used to hold property store values that are not localized
     * key: Field Name
     * value: Data
     */
    std::map<qcc::String, MsgArg> propertyStore;

    struct CaseInsensitiveCompare {
        struct CaseInsensitiveCharCompare {
            bool operator()(const char& lhs, const char& rhs)
            {
                return std::tolower(lhs) < std::tolower(rhs);
            }
        };

        bool operator()(const qcc::String& lhs, const qcc::String& rhs) const
        {
            return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                                rhs.begin(), rhs.end(),
                                                CaseInsensitiveCharCompare());
        }
    };

    MsgArgTableTranslator defaultTranslator;
    Translator* translator;

    /**
     * The pseudo-language of a Field Name.  Currently this is always the
     * empty string, and is used to allow a translator to "translate"
     * a field name into its description by identifying the source text
     * as field name.
     */
    qcc::String keyLanguage;
};

}
#endif //_ALLJOYN_ABOUTDATAINTERNAL_H
