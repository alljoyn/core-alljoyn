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

#include <alljoyn/Translator.h>

namespace ajn {

// Find the best matching language tag, using the lookup algorithm in
// RFC 4647 section 3.4.  This algorithm requires that the "supported"
// languages be the least specific they can (e.g., "en" in order to match
// both "en" and "en-US" if requested), and the "requested" language be
// the most specific it can (e.g., "en-US" in order to match either "en-US"
// or "en" if supported).
void Translator::GetBestLanguage(const char* requested, const qcc::String& defaultLanguage, qcc::String& ret)
{
    if ((requested != NULL) && (*requested != 0)) {
        size_t numTargetLanguages = NumTargetLanguages();
        qcc::String languageToCheck(requested);
        qcc::String targetLanguage;
        for (;;) {
            // Look for a supported language matching the language to check.
            for (size_t index = 0; index < numTargetLanguages; index++) {
                GetTargetLanguage(index, targetLanguage);
                if (strcasecmp(targetLanguage.c_str(), languageToCheck.c_str()) == 0) {
                    ret = targetLanguage;
                    return;
                }
            }

            // Drop the last subtag and try again.
            size_t pos = languageToCheck.find_last_of('-');
            if (pos == qcc::String::npos) {
                break;
            }
            languageToCheck.erase(pos);
        }
    }

    // No match found, so return the default language.
    ret = defaultLanguage;
    return;
}

size_t LookupTableTranslator::NumTargetLanguages()
{
    return supportedLanguages.size();
}

void LookupTableTranslator::GetTargetLanguage(size_t index, qcc::String& ret)
{
    supportedLanguagesIterator it = supportedLanguages.begin();
    for (size_t count = 0; count < index; it++, count++);
    ret = *it;
}

QStatus LookupTableTranslator::AddTargetLanguage(const char* language, bool* added)
{
    std::pair<supportedLanguagesIterator, bool> ret = supportedLanguages.insert(language);
    if (added != NULL) {
        *added = ret.second;
    }
    return ER_OK;
}

const char* StringTableTranslator::Translate(const char* sourceLanguage, const char* targetLanguage, const char* sourceText)
{
    QCC_UNUSED(sourceLanguage);
    return localizedStore[sourceText][targetLanguage].c_str();
}

const char* MsgArgTableTranslator::Translate(const char* sourceLanguage, const char* targetLanguage, const char* sourceText)
{
    QCC_UNUSED(sourceLanguage);
    char* result;
    QStatus status = localizedStore[sourceText][targetLanguage].Get("s", &result);
    return (status == ER_OK) ? result : nullptr;
}

QStatus MsgArgTableTranslator::TranslateToMsgArg(const char* sourceLanguage, const char* targetLanguage, const char* sourceText, ajn::MsgArg*& value)
{
    QCC_UNUSED(sourceLanguage);
    value = &localizedStore[sourceText][targetLanguage];
    return (value->typeId == ALLJOYN_INVALID) ? ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE : ER_OK;
}

QStatus StringTableTranslator::AddStringTranslation(const char* id, const char* value, const char* language)
{
    if (supportedLanguages.find(language) == supportedLanguages.end()) {
        supportedLanguages.insert(language);
    }
    localizedStore[id][language] = value;
    return ER_OK;
}

QStatus MsgArgTableTranslator::AddMsgArgTranslation(const char* id, const MsgArg* value, const char* language)
{
    if (supportedLanguages.find(language) == supportedLanguages.end()) {
        supportedLanguages.insert(language);
    }
    localizedStore[id][language] = *value;
    return ER_OK;
}

const char* StringTableTranslator::GetFieldId(size_t index)
{
    localizedStoreIterator it = localizedStore.begin();
    for (size_t count = 0; count < index; it++, count++);
    return it->first.c_str();
}

const char* MsgArgTableTranslator::GetFieldId(size_t index)
{
    localizedStoreIterator it = localizedStore.begin();
    for (size_t count = 0; count < index; it++, count++);
    return it->first.c_str();
}

}
