/**
 * @file
 *
 * Non-STL version of string.
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
#ifndef _QCC_STRING_H
#define _QCC_STRING_H

#include <qcc/platform.h>
#include <string.h>
#include <string>
#include <iostream>

namespace qcc {

/**
 * String is a heap-allocated array of bytes whose life-cycle is managed
 * through reference counting. When all references to a qcc::String instance
 * go out of scope or are deleted, then the underlying heap-allocated storage
 * is freed.
 */
class String {
    std::string s;

  public:
    /** String index constant indicating "past the end" */
    static const std::string::size_type npos;

    /** String iterator type */
    typedef std::string::iterator iterator;

    /** const String iterator type */
    typedef std::string::const_iterator const_iterator;

    /** String size_type type */
    typedef std::string::size_type size_type;

    /**
     * Construct an empty string.
     */
    String() : s() { }

    /**
     * Construct a single character string.
     *
     * @param c         Initial value for string
     * @param sizeHint  Optional size hint for initial allocation.
     */
    QCC_DEPRECATED(String(char c, size_t sizeHint)) { s.reserve(sizeHint); s.assign(1, c); }
    QCC_DEPRECATED(String(char c)) : s(1, c) { }

    /**
     * Construct a String with n copies of char.
     *
     * @param n         Number of chars in string.
     * @param c         Character used to fill string.
     * @param sizeHint  Optional size hint for initial allocation.
     */
    QCC_DEPRECATED(String(size_type n, char c, size_t sizeHint)) {
        s.reserve(sizeHint);
        s.assign(n, c);
    }
    String(size_type n, char c) : s(n, c) { }

    /**
     * Construct a string from a const char*
     *
     * @param str        char* array to use as initial value for String.
     * @param strLen     Length of string or 0 if str is null terminated
     * @param sizeHint   Optional size hint used for initial malloc if larger than str length.
     */
    QCC_DEPRECATED(String(const char* str, size_type strLen, size_t sizeHint));
    String(const char* str, size_type strLen);
    String(const char* str);

    /**
     * Copy Constructor
     *
     * @param str        String to copy
     */
    String(const String& str) : s(str.s) { }

    String(std::string&& str) : s(str) { }
    String(const std::string& str) : s(str) { }

    String(const const_iterator& start, const const_iterator& end) : s(start, end) { }


    /** Destructor */
    virtual ~String() { }

    operator const std::string & () const { return s; }
    operator std::string & () { return s; }

    /** Assignment operator */
    std::string operator=(const String& assignFromMe) { return s = assignFromMe.s; }

    /**
     * Assign a value to a string
     *
     * @param str  Value to assign to string.
     * @param len  Number of characters to assign or 0 to insert up to first nul byte in str.
     * @return  Reference to this string.
     */
    std::string assign(const char* str, size_type len);

    /**
     * Assign a nul-terminated string value to a string
     *
     * @param str  Value to assign to string.
     * @return  Reference to this string.
     */
    std::string assign(const char* str);

    /**
     * Get the current storage capacity for this string.
     *
     * @return  Amount of storage allocated to this string.
     */
    size_type capacity() const { return s.capacity(); }

    /**
     * Get an iterator to the beginning of the string.
     *
     * @return iterator to start of string
     */
    iterator begin() { return s.begin(); }

    /**
     * Get an iterator to the end of the string.
     *
     * @return iterator to end of string.
     */
    iterator end() { return s.end(); }

    /**
     * Get a const_iterator to the beginning of the string.
     *
     * @return iterator to start of string
     */
    const_iterator begin() const { return s.cbegin(); }

    /**
     * Get an iterator to the end of the string.
     *
     * @return iterator to end of string.
     */
    const_iterator end() const { return s.cend(); }

    /**
     * Clear contents of string.
     *
     * @param sizeHint   Allocation size hint used if string must be reallocated.
     */
    QCC_DEPRECATED(void clear(size_t sizeHint)) { s.clear(); s.resize(sizeHint); }
    void clear() { s.clear(); }

    /**
     * @deprecated
     * Unless you are working with passwords or cryptographic keys you should probably not be
     * calling this function and should call String::clear() instead.
     *
     * Clears the context of a string by zeroing out the internal string and setting the length to
     * zero. This function is intended for use by security related functions for zeroing out
     * sensitive information such as passwords and cryptographic keys immediately after they have
     * been used to minimize the time that sensitive information is in memory. This function has
     * the side effect of clearing all copies of this string that may have result from string
     * assignment operations. To aid in verifying the behavior is as expected this function
     * returns a count of the number of strings that reference the same internal string data. If
     * this value is not zero then there are other copies of the string that were also cleared.
     * If this happens it was most likely due to a coding error.
     *
     * @return  The number of other string instances that were cleared as a side-effect of clearing
     *          this string.
     */
    QCC_DEPRECATED(size_type secure_clear());

    /**
     * Append a string or substring to string. This function will append all characters up to the
     * specified length including embedded nuls.
     *
     * @param str  Value to append to string.
     * @param len  Number of characters to append or 0 to insert up to first nul byte in str.
     * @return  Reference to this string.
     */
    std::string append(const char* str, size_type len);
    std::string append(const char* str);

    /**
     * Append a string to another to string.
     *
     * @param str  Value to append to string.
     * @return  Reference to this string.
     */
    std::string append(const String& str) { return s.append(str.s); }

    /**
     * Append a character N times to the string.
     *
     * @param n  Number of copies of 'c' to append.
     * @param c  Character to append to string.
     * @return  Reference to this string.
     */
    std::string append(size_type n, char c) { return s.append(n, c); }

    /**
     * Append a single character to string.
     *
     * @param c Character to append to string.
     * @return  Reference to this string.
     */
    QCC_DEPRECATED(String append(const char c)) { return s.append(1, c); }

    /**
     * Erase a range of chars from string.
     *
     * @param pos    Offset first char to erase.
     * @param n      Number of chars to erase.
     * @return  Reference to this string.
     */
    std::string erase(size_type pos = 0, size_type n = npos) { return s.erase(pos, n); }

    /**
     * Resize string by appending chars or removing them to make string a specified size.
     *
     * @param n     New size for string.
     * @param c     Character to append to string if string size increases.
     */
    void resize(size_type n, char c = ' ') { s.resize(n, c); }

    /**
     * Set storage space for this string.
     * The new capacity will be the larger of the current size and the specified new capacity.
     *
     * @param newCapacity   The desired capacity for this string. May be greater or less than current capacity.
     */
    void reserve(size_type newCapacity) { s.reserve(newCapacity); }

    /**
     * Push a single character to the end of the string.
     *
     * @param c  Char to push
     */
    void push_back(char c) { s.push_back(c); }

    /**
     * Append a character.
     *
     * @param c Character to append to string.
     * @return  Reference to this string.
     */
    std::string operator+=(const char c) { return s += c; }

    /**
     * Append to string.
     *
     * @param str  Value to append to string.
     * @return  Reference to this string.
     */
    std::string operator+=(const char* str) { return append(str); }

    /**
     * Append to string.
     *
     * @param str  Value to append to string.
     * @return  Reference to this string.
     */
    std::string operator+=(const String& str) { return s += str.s; }

    std::string operator+=(const std::string& str) { return s += str; }

    /**
     * Insert characters into string at position.
     *
     * @param pos   Insert position.
     * @param str   Character string to insert.
     * @param len   Optional number of chars to insert.
     * @return  Reference to the string.
     */
    std::string insert(size_type pos, const char* str, size_type len);

    std::string insert(size_type pos, const char* str);

    /**
     * Return true if string is equal to this string.
     *
     * @param str  %String to compare against this string.
     * @return true iff other is equal to this string.
     */
    bool operator==(const String& str) const { return s == str.s; }

    /**
     * @overload
     * @copydoc String::operator==
     */
    bool operator==(const char* str) const { return compare(str) == 0; }

    /**
     * Return true if string is not equal to this string.
     *
     * @param str  String to compare against this string.
     * @return true iff other is not equal to this string.
     */
    bool operator!=(const String& str) const { return s != str.s; }
    bool operator!=(const std::string& str) const { return s != str; }

    /**
     * @overload
     * @copydoc String::operator!=
     */
    bool operator!=(const char* str) const { return !operator==(str); }

    /**
     * Return true if this string is less than other string.
     *
     * @param str  String to compare against this string.
     * @return true iff this string is less than other string
     */
    bool operator<(const String& str) const { return s < str.s; }

    /**
     * @overload
     * @copydoc String::operator<
     */
    bool operator<(const char* str) const { return compare(str) < 0; }

    /**
     * Get a reference to the character at a given position.
     * If pos >= size, then 0 will be returned.
     *
     * @param pos    Position offset into string.
     * @return  Reference to character at pos.
     */
    char& operator[](size_type pos) { return s[pos]; }

    /**
     * Get a character at a given position. This function performs no range checking so the caller
     * is responsible for checking that pos is less than the size of the the string.
     *
     * @param pos    Position offset into string.
     * @return       The character at pos.
     */
    char operator[](size_type pos) const { return s[pos]; }

    /**
     * Get the size of the string.
     *
     * @return size of string.
     */
    size_type size() const { return s.size(); }

    /**
     * Get the length of the string.
     *
     * @return size of string.
     */
    size_type length() const { return s.length(); }

    /**
     * Get the null termination char* representation for this String.
     *
     * @return Null terminated string.
     */
    const char* c_str() const { return s.c_str(); }

    /**
     * Get the not-necessarily null termination char* representation for this String.
     *
     * @return Null terminated string.
     */
    const char* data() const { return s.data(); }

    /**
     * Return true if string contains no chars.
     *
     * @return true iff string is empty
     */
    bool empty() const { return s.empty(); }

    /**
     * Find first occurrence of null terminated string within this string.
     *
     * @param str  String to find.
     * @param pos  Optional starting position (in this string) for search.
     * @return     Position of first occurrence of c within string or npos if not found.
     */
    size_type find(const char* str, size_type pos = 0) const {
        return s.find(str, pos);
    }

    /**
     * Find first occurrence of string within this string.
     *
     * @param str  String to find within this string instance.
     * @param pos  Optional starting position (in this string) for search.
     * @return     Position of first occurrence of c within string or npos if not found.
     */
    size_type find(const qcc::String& str, size_type pos = 0) const {
        return s.find(str.s, pos);
    }

    /**
     * Find first occurrence of character within string.
     *
     * @param c    Charater to find.
     * @param pos  Optional starting position for search.
     * @return     Position of first occurrence of c within string or npos if not found.
     */
    size_type find_first_of(const char c, size_type pos = 0) const {
        return s.find_first_of(c, pos);
    }

    /**
     * Find last occurrence of character within string in range [0, pos).
     *
     * @param c    Character to find.
     * @param pos  Optional starting position for search (one past end of substring to search).
     * @return     Position of last occurrence of c within string or npos if not found.
     */
    size_type find_last_of(const char c, size_type pos = npos) const {
        return s.find_last_of(c, pos);
    }

    /**
     * Find first occurence of any of a set of characters within string.
     *
     * @param inChars    Array of characters to look for in this string.
     * @param pos        Optional starting position within this string for search.
     * @return           Position of first occurrence of one of inChars within string or npos if not found.
     */
    size_type find_first_of(const char* inChars, size_type pos = 0) const {
        return s.find_first_of(inChars, pos);
    }

    /**
     * Find last occurence of any of a set of characters within string.
     *
     * @param inChars    Array of characters to look for in this string.
     * @param pos        Optional starting position within this string for search.
     * @return           Position of last occurrence of one of inChars within string or npos if not found.
     */
    size_type find_last_of(const char* inChars, size_type pos = npos) const {
        return s.find_last_of(inChars, pos);
    }

    /**
     * Find first occurrence of a character NOT in a set of characters within string.
     *
     * @param setChars   Array of characters to (NOT) look for in this string.
     * @param pos        Optional starting position within this string for search.
     * @return           Position of first occurrence a character not in setChars or npos if none exists.
     */
    size_type find_first_not_of(const char* setChars, size_type pos = 0) const {
        return s.find_first_not_of(setChars, pos);
    }

    /**
     * Find last occurrence of a character NOT in a set of characters within string range [0, pos).
     *
     * @param setChars   Array of characters to (NOT) look for in this string.
     * @param pos        Position one past the end of the character of the substring that should be examined or npos for entire string.
     * @return           Position of first occurrence a character not in setChars or npos if none exists.
     */
    size_type find_last_not_of(const char* setChars, size_type pos = npos) const {
        return s.find_last_not_of(setChars, pos);
    }

    /**
     * Return a substring of this string.
     *
     * @param  pos  Starting position of substring.
     * @param  n    Number of bytes in substring.
     * @return  Substring of this string.
     */
    std::string substr(size_type pos = 0, size_type n = npos) const { return s.substr(pos, n); }

    /**
     * Return a substring of this string with the order of the characters reversed.
     *
     * @param  pos  Starting position of substring.
     * @param  n    Number of bytes in substring.
     * @return  The reversed substring of this string.
     */
    QCC_DEPRECATED(std::string revsubstr(size_type pos = 0, size_type n = npos) const);

    /**
     * Compare this string with other.
     *
     * @param other   String to compare with.
     * @return  &lt;0 if this string is less than other, &gt;0 if this string is greater than other, 0 if equal.
     */
    int compare(const String& other) const { return s.compare(other.s); }

    /**
     * Compare a substring of this string with a substring of other.
     *
     * @param pos       Start position of this string.
     * @param n         Number of characters of this string to use for compare.
     * @param other     String to compare with.
     * @param otherPos  Start position of other string.
     * @param otherN    Number of characters of other string to use for compare.
     *
     * @return  &lt;0 if this string is less than other, &gt;0 if this string is greater than other, 0 if equal.
     */
    int compare(size_type pos,
                size_type n,
                const String& other,
                size_type otherPos,
                size_type otherN) const {
        return s.compare(pos, n, other.s, otherPos, otherN);
    }

    /**
     * Compare a substring of this string with other.
     *
     * @param pos    Start position of this string.
     * @param n      Number of characters of this string to use for compare.
     * @param other  String to compare with.
     * @return  &lt;0 if this string is less than other, &gt;0 if this string is greater than other, 0 if equal.
     */
    int compare(size_type pos, size_type n, const String& other) const {
        return s.compare(pos, n, other.s);
    }

    /**
     * @overload
     * Compare a substring of this string with other.
     *
     * @param pos    Start position of this string.
     * @param n      Number of characters of this string to use for compare.
     * @param other  String to compare with.
     * @return  &lt;0 if this string is less than other, &gt;0 if this string is greater than other, 0 if equal.
     */
    int compare(size_type pos, size_type n, const char* other) const {
        return s.compare(pos, n, other);
    }

    /**
     * Compare this string with an array of chars.
     *
     * @param str   Nul terminated array of chars to compare against this string.
     * @return  &lt;0 if this string is less than str, &gt;0 if this string is greater than str, 0 if equal.
     */
    int compare(const char* str) const { return s.compare(str); }

    /**
     * Returns a reference to the empty string
     */
    static const String& Empty;

  private:
    static void Init();
    static void Shutdown();
    friend class StaticGlobals;
};

}

/**
 * Global "+" operator for qcc::String
 *
 * @param   s1  String to be concatenated.
 * @param   s2  String to be concatenated.
 * @return  Concatenated s1 + s2.
 */
inline qcc::String AJ_CALL operator+(const qcc::String& s1, const qcc::String& s2) {
    return static_cast<const std::string>(s1) + static_cast<const std::string>(s2);
}

/**
 * Global "<<" operator for qcc::String
 *
 * @param   os  Output stream to print string contents on
 * @param   str  String to be printed.
 * @return  Output stream
 */
inline std::ostream& AJ_CALL operator<<(std::ostream& os, const qcc::String& str) {
    return os << static_cast<const std::string>(str);
}

inline bool AJ_CALL operator==(const std::string& s1, const qcc::String& s2) {
    return s1 == static_cast<std::string>(s2);
}
inline bool AJ_CALL operator!=(const std::string& s1, const qcc::String& s2) {
    return s1 == static_cast<std::string>(s2);
}
inline bool AJ_CALL operator<(const std::string& s1, const qcc::String& s2) {
    return s1 < static_cast<std::string>(s2);
}
inline bool AJ_CALL operator>(const std::string& s1, const qcc::String& s2) {
    return s1 > static_cast<std::string>(s2);
}
inline bool AJ_CALL operator<=(const std::string& s1, const qcc::String& s2) {
    return s1 <= static_cast<std::string>(s2);
}
inline bool AJ_CALL operator>=(const std::string& s1, const qcc::String& s2) {
    return s1 >= static_cast<std::string>(s2);
}

#endif
