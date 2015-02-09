/**
 * @file
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

#ifndef _QCC_STRINGMAPKEY_H
#define _QCC_STRINGMAPKEY_H

#include <qcc/platform.h>

#include <qcc/Util.h>
#include <qcc/String.h>
#include <string.h>

#include <qcc/STLContainer.h>

namespace qcc {

/**
 * StringMapKey is useful when creating a std:map that wants a std::string
 * as its key. It is preferable to using a std::string directly since
 * doing so would require all lookup operations on the map to first create
 * a std::string (rather than using a simple const char*).
 */
class StringMapKey {
  public:
    /**
     * Create a backed version of the StringMapKey
     * Typically, this constructor is used when inserting into a map
     * since it causes storage to be allocated for the string
     *
     * @param key   String whose value will be copied into StringMapKey
     */
    StringMapKey(const qcc::String& key) : charPtr(NULL), str(key) { }

    /**
     * Create an unbacked version of the StringMapKey
     * Typically, this constructor is used when forming a key to pass to
     * map::find(), etc. The StringMapKey is a simple container for the
     * passed in char*. The char* arg to this constructor must remain
     * valid for the life of the StringMapKey.
     *
     * @param key   char* whose value (but not contents) will be stored
     *              in the StringMapKey.
     */
    StringMapKey(const char* key) : charPtr(key), str() { }

    /**
     * Get a char* representation of this StringKeyMap
     *
     * @return  char* representation this StringKeyMap
     */
    inline const char* c_str() const { return charPtr ? charPtr : str.c_str(); }

    /**
     * Return true if StringMapKey is empty.
     *
     * @return true iff StringMapKey is empty.
     */
    inline bool empty() const { return charPtr ? charPtr[0] == '\0' : str.empty(); }

    /**
     * Return the size of the contained string.
     *
     * @return size of the contained string.
     */
    inline size_t size() const { return charPtr ? strlen(charPtr) : str.size(); }

    /**
     * Less than operation
     */
    inline bool operator<(const StringMapKey& other) const { return ::strcmp(c_str(), other.c_str()) < 0; }

    /**
     * Equals operation
     */
    inline bool operator==(const StringMapKey& other) const { return ::strcmp(c_str(), other.c_str()) == 0; }

  private:
    const char* charPtr;
    qcc::String str;
};

}  // End of qcc namespace




namespace std {
/**
 * Functor to compute StrictWeakOrder
 */
template <>
struct less<qcc::StringMapKey> {
    inline bool operator()(const qcc::StringMapKey& a, const qcc::StringMapKey& b) const { return ::strcmp(a.c_str(), b.c_str()) < 0; }
};
}  // End of std namespace



_BEGIN_NAMESPACE_CONTAINER_FOR_HASH
/**
 * Functor to compute a hash for StringMapKey suitable for use with
 * std::unordered_map, std::unordered_set, std::hash_map, std::hash_set.
 */
template <>
struct hash<qcc::StringMapKey> {
    inline size_t operator()(const qcc::StringMapKey& k) const { return qcc::hash_string(k.c_str()); }
};
_END_NAMESPACE_CONTAINER_FOR_HASH



#endif
