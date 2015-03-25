/**
 * @file
 * Translator is an abstract base class (interface) implemented by users of the
 * AllJoyn API in order to provide org.allseen.Introspectable descriptions in more than just one language
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

namespace ajn {

/**
 * Abstract base class that provides translations of descriptions returned from
 * org.allseen.Introspectable.IntrospectWithDescription.
 */
class Translator {
  public:

    virtual ~Translator() { }

    /**
     * Retrieve the number of target language tags this Translator
     * can translate into.
     * @return size_t The number of target language tags
     */
    virtual size_t NumTargetLanguages() = 0;

    /**
     * Retrieve the n'th target language tag
     * @param index The index of the language tag to retrieve
     * @param ret The returned value
     */
    virtual void GetTargetLanguage(size_t index, qcc::String& ret) = 0;

    /**
     * Translate source from sourceLanguage into targetLanguage.
     * If this Translator does not have a translation for the given
     * parameters it should return NULL.
     *
     * @param sourceLanguage The language tag of the text in source
     * @param targetLanguage The language tag to translate into
     * @param source The source text to translate
     * @return The translated text or NULL
     */
    virtual const char* Translate(const char* sourceLanguage, const char* targetLanguage, const char* source) {
        QCC_UNUSED(sourceLanguage);
        QCC_UNUSED(targetLanguage);
        QCC_UNUSED(source);
        return NULL;
    }

    /**
     * Translate source from sourceLanguage into targetLanguage.
     * If this Translator does not have a translation for the given
     * parameters it should return NULL. This version of the
     * function is designed for imeplementations that return dynamically allocated strings.
     * The string should be copied into buffer and buffer.c_str() must be returned. When alljoyn
     * finishes using the string buffer will be free'ed
     *
     * @param sourceLanguage The language tag of the text in source
     * @param targetLanguage The language tag to translate into
     * @param source The source text to translate
     * @param buffer A buffer to hold the dynamically allocated string returned by this function
     * @return The translated text or NULL
     */
    virtual const char* Translate(const char* sourceLanguage,
                                  const char* targetLanguage, const char* source, qcc::String& buffer) {
        QCC_UNUSED(buffer);
        return Translate(sourceLanguage, targetLanguage, source);
    }
};

}

#endif
