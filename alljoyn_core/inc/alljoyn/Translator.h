/**
 * @file
 * Translator is an abstract base class (interface) implemented by users of the
 * AllJoyn API in order to provide text in more than just one language. This
 * is used with APIs such as IntrospectWithDescription and GetAboutData.
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
#ifndef _ALLJOYN_DESCRIPTIONTRANSLATOR_H
#define _ALLJOYN_DESCRIPTIONTRANSLATOR_H

#ifndef __cplusplus
#error Only include Translator.h in C++ code.
#endif

#include <qcc/String.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/Status.h>
#include <cctype>
#include <map>
#include <set>

namespace ajn {

/**
 * Abstract base class that provides translations of text.
 */
class Translator {
  public:

    /** Destructor */
    virtual ~Translator() { }

    /**
     * Retrieve the number of target language tags this Translator
     * can translate into.
     *
     * @return size_t The number of target language tags
     */
    virtual size_t NumTargetLanguages() = 0;

    /**
     * Retrieve the n'th target language tag.
     *
     * @param[in] index The index of the language tag to retrieve
     * @param[out] ret The returned value
     */
    virtual void GetTargetLanguage(size_t index, qcc::String& ret) = 0;

    /**
     * Add a language to the set of supported target languages.
     *
     * @param[in] language The IETF language tag specified by RFC 5646
     * @param[out] added Returns true if added, false if already present
     * @return
     *  - #ER_OK on success
     *  - #ER_NOT_IMPLEMENTED if translator does not support adding target languages.
     */
    virtual QStatus AddTargetLanguage(const char* language, bool* added = NULL) {
        QCC_UNUSED(language);
        QCC_UNUSED(added);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Translate an id or source text from sourceLanguage into text in the
     * given targetLanguage. If this Translator does not have a translation
     * for the given parameters, it should return NULL.
     *
     * @param[in] sourceLanguage The language tag of the text in sourceText. If
     *  sourceLanguage is NULL or empty, then sourceText is simply an id used
     *  for lookup.
     * @param[in] targetLanguage The language tag to translate into
     * @param[in] sourceText The source text to translate or id to look up
     * @return The translated text or NULL
     */
    virtual const char* Translate(const char* sourceLanguage, const char* targetLanguage, const char* sourceText) {
        QCC_UNUSED(sourceLanguage);
        QCC_UNUSED(targetLanguage);
        QCC_UNUSED(sourceText);
        return NULL;
    }

    /**
     * Translate an id or source text from sourceLanguage into text in the
     * given targetLanguage. If this Translator does not have a translation
     * for the given parameters, it should return NULL. This version of the
     * function is designed for implementations that return dynamically allocated strings.
     * The string should be copied into buffer and buffer.c_str() must be returned. When alljoyn
     * finishes using the string buffer will be free'ed
     *
     * @param[in] sourceLanguage The language tag of the text in sourceText. If
     *  sourceLanguage is NULL or empty, then sourceText is simply an id used
     *  for lookup.
     * @param[in] targetLanguage The language tag to translate into
     * @param[in] sourceText The source text to translate or id to look up
     * @param[out] buffer A buffer to hold the dynamically allocated string returned by this function
     * @return The translated text or NULL
     */
    virtual const char* Translate(const char* sourceLanguage,
                                  const char* targetLanguage, const char* sourceText, qcc::String& buffer) {
        QCC_UNUSED(buffer);
        return Translate(sourceLanguage, targetLanguage, sourceText);
    }

    /**
     * Translate an id or source text from sourceLanguage into text in the
     * given targetLanguage. If this Translator does not support MsgArgs it
     * should return NULL.
     * This version of the function is designed for implementations that
     * return a pointer to a MsgArg that will not go away. This is required
     * by the AboutData::GetField() API.
     *
     * @param[in] sourceLanguage The language tag of the text in sourcetext. If
     *  sourceLanguage is NULL or empty, then sourceText is simply an id used
     *  for lookup.
     * @param[in] targetLanguage The language tag to translate into
     * @param[in] sourceText The source text to translate
     * @param[out] msgarg Returns the MsgArg containing the translation
     * @return
     *  - #ER_OK on success
     *  - #ER_NOT_IMPLEMENTED if translator does not support MsgArgs
     */
    virtual QStatus TranslateToMsgArg(const char* sourceLanguage,
                                      const char* targetLanguage, const char* sourceText, MsgArg*& msgarg) {
        QCC_UNUSED(sourceLanguage);
        QCC_UNUSED(targetLanguage);
        QCC_UNUSED(sourceText);
        QCC_UNUSED(msgarg);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Add new localized text.
     *
     * @param[in] id The id of the localized text to add.
     * @param[in] value The localized text to be associated with the id.
     * @param[in] language The IETF language tag specified by RFC 5646.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_NOT_IMPLEMENTED if translator does not support adding target languages.
     */
    virtual QStatus AddStringTranslation(const char* id, const char* value, const char* language) {
        QCC_UNUSED(id);
        QCC_UNUSED(value);
        QCC_UNUSED(language);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Add new localized text.
     *
     * @param[in] id The id of the localized text to add.
     * @param[in] value The localized text to be associated with the id.
     * @param[in] language The IETF language tag specified by RFC 5646.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_NOT_IMPLEMENTED if translator does not support adding target languages or the MsgArg type.
     */
    virtual QStatus AddMsgArgTranslation(const char* id, const MsgArg* value, const char* language) {
        if (value->typeId != ALLJOYN_STRING) {
            return ER_NOT_IMPLEMENTED;
        }
        return AddStringTranslation(id, value->v_string.str, language);
    }

    /**
     * Get the best matching language according to RFC 4647 section 3.4.
     *
     * @param[in] The requested IETF language range.
     * @param[in] The default language to use.
     * @param[out] ret The returned value
     */
    virtual void GetBestLanguage(const char* requested, const qcc::String& defaultLanguage, qcc::String& ret);
};

/**
 * Abstract base class that provides translations of text using a lookup
 * table that maps a "field id" and a target language to a description
 * in the target language corresponding to the indicated field.  Each
 * unique string to translate should have its own id.  For example,
 * AboutData has ids for manufacturer, description, etc.
 */
class LookupTableTranslator : public Translator {
  public:
    /**
     * @copydoc Translator::NumTargetLanguages
     * @see Translator::NumTargetLanguages
     */
    virtual size_t NumTargetLanguages();

    /**
     * @copydoc Translator::GetTargetLanguage
     * @see Translator::GetTargetLanguage
     */
    virtual void GetTargetLanguage(size_t index, qcc::String& ret);

    /**
     * @copydoc Translator::AddTargetLanguage
     * @see Translator::AddTargetLanguage
     */
    virtual QStatus AddTargetLanguage(const char* language, bool* added);

    /**
     * Retrieve the number of field ids this Translator has translations for.
     * @return size_t The number of field ids
     */
    virtual size_t NumFields() = 0;

    /**
     * Retrieve the n'th field id
     * @param index The index of the field id to retrieve
     * @return The n'th field id
     */
    virtual const char* GetFieldId(size_t index) = 0;

  protected:
    /**
     * Struct to handle case insensitive comparison operator used to make sure
     * the case of language tags are ignored when adding, sorting and reading them.
     */
    struct CaseInsensitiveCompare {
        /**
         * inner case insensitive comparison struct used by the
         * std::lexicographical_compare function.
         */
        struct CaseInsensitiveCharCompare {
            /**
             * CaseInsensitive comparison parentheses operator for individual chars
             * @param lhs char value to compare
             * @param rhs char value to compare
             * @return true if the case insensitive comparison of lhs < rhs
             */
            bool operator()(const char& lhs, const char& rhs)
            {
                return std::tolower(lhs) < std::tolower(rhs);
            }
        };

        /**
         * CaseInsensitive comparison parentheses operator for Strings
         * @param lhs String to compare
         * @param rhs String to to compare
         * @return true if the case insensitive comparison of lhs < rhs
         */
        bool operator()(const qcc::String& lhs, const qcc::String& rhs) const
        {
            return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                                rhs.begin(), rhs.end(),
                                                CaseInsensitiveCharCompare());
        }
    };

    /**
     * Local member variable for supported target languages.
     */
    std::set<qcc::String, CaseInsensitiveCompare> supportedLanguages;

    /**
     * typedef supported languages iterator
     */
    typedef std::set<qcc::String, CaseInsensitiveCompare>::iterator supportedLanguagesIterator;
};

/**
 * Class that provides translations of text using a lookup table where
 * each translated string is stored as a simple string.  This can be used
 * for introspection descriptions, and potentially other APIs.
 */
class StringTableTranslator : public LookupTableTranslator {
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
    virtual const char* GetFieldId(size_t index);

    /**
     * @see Translator::Translate
     */
    virtual const char* Translate(const char* sourceLanguage, const char* targetLanguage, const char* sourceText);

    /**
     * @see Translator::AddStringTranslation
     */
    virtual QStatus AddStringTranslation(const char* id, const char* value, const char* language);

    /**
     * Determine if the table is empty.
     *
     * @return  Returns true if the table is empty.
     */
    bool IsEmpty() const { return localizedStore.empty(); }

  protected:
    /**
     * Local member variable mapping a field id to a set of translations
     * in various languages.
     */
    std::map<qcc::String, std::map<qcc::String, qcc::String, CaseInsensitiveCompare> > localizedStore;

    /**
     * typedef localized store iterator
     */
    typedef std::map<qcc::String, std::map<qcc::String, qcc::String, CaseInsensitiveCompare> >::iterator localizedStoreIterator;
};

}

#endif
