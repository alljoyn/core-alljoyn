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
#ifndef _ALLJOYN_ABOUTDATAINTERNAL_H
#define _ALLJOYN_ABOUTDATAINTERNAL_H
#include <map>
#include <set>

#include <alljoyn/AboutData.h>
#include <qcc/Mutex.h>
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
    std::map<qcc::String, std::map<qcc::String, MsgArg, CaseInsensitiveCompare>
> localizedStore;

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

    /**
     * mutex lock to protect the property store.
     */
    qcc::Mutex propertyStoreLock;
};

}
#endif //_ALLJOYN_ABOUTDATAINTERNAL_H