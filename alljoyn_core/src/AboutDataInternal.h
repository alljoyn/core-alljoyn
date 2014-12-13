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
 * Class used to hold internal values for the AboutData class
 */
class AboutData::Internal {
    friend class AboutData;
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

        bool operator()(const qcc::String& lhs, const qcc::String& rhs)
        {
            return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                                rhs.begin(), rhs.end(),
                                                CaseInsensitiveCharCompare());
        }
    };

    /**
     * key: Field Name
     * value: map of language / Data
     */
    std::map<qcc::String, std::map<qcc::String, MsgArg, CaseInsensitiveCompare> > localizedPropertyStore;

    /**
     * typedef const iterator
     */
    typedef std::map<qcc::String, std::map<qcc::String, MsgArg, CaseInsensitiveCompare> >::const_iterator localizedPropertyStoreConstIterator;

    /**
     * local member variable for supported languages
     */
    std::set<qcc::String, CaseInsensitiveCompare> supportedLanguages;

    /**
     * typedef supported languages iterator
     */
    typedef std::set<qcc::String, CaseInsensitiveCompare>::iterator supportedLanguagesIterator;

    /**
     * mutex lock to protect the property store.
     */
    qcc::Mutex propertyStoreLock;
};
}
#endif //_ALLJOYN_ABOUTDATAINTERNAL_H
