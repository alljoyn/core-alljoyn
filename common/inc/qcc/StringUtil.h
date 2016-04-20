/**
 * @file
 *
 * This file defines string related utility functions
 */

/******************************************************************************
 *
 *
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

#ifndef _QCC_STRINGUTIL_H
#define _QCC_STRINGUTIL_H

#include <vector>

#include <qcc/platform.h>
#include <qcc/String.h>

namespace qcc {

/**
 * Convert byte array to hex representation.
 *
 * @param inBytes    Pointer to byte array.
 * @param len        Number of bytes.
 * @param toLower    TRUE to use a-f, FALSE for A-F
 * @param separator  Separator to use between each byte
 * @return Hex string representation of inBytes.
 */
qcc::String AJ_CALL BytesToHexString(const uint8_t* inBytes, size_t len, bool toLower = false, char separator = 0);


/**
 * Convert hex string to a byte array representation.
 *
 * @param hex        String containing hex encoded data
 * @param outBytes   Pointer to byte array.
 * @param len        Size of byte array
 * @param separator  Separator to expect between each byte
 * @return Number of bytes written
 */
size_t AJ_CALL HexStringToBytes(const qcc::String& hex, uint8_t* outBytes, size_t len, char separator = 0);


/**
 * Convert hex string to a string of bytes
 *
 * @param hex        String containing hex encoded data
 * @param separator  Separator to expect between each byte
 * @return A string containing the converted bytes or an empty string if the conversion failed.
 */
qcc::String AJ_CALL HexStringToByteString(const qcc::String& hex, char separator = 0);


/**
 * Generate a random hex string.
 */
qcc::String AJ_CALL RandHexString(size_t len, bool toLower = false);


/**
 * Convert uint32_t to a string.
 *
 * @param num     Number to convert.
 * @param base    Base (radix) for output string. (Must be between 1 and 16 inclusive). Defaults to 10.
 * @param width   Minimum amount of space in string the conversion will use.
 * @param fill    Fill character.
 * @return  String representation of num.
 */
qcc::String AJ_CALL U32ToString(uint32_t num, unsigned int base = 10, size_t width = 1, char fill = ' ');

/**
 * Convert int32_t to a string.
 *
 * @param num     Number to convert.
 * @param base    Base (radix) for output string. (Must be between 1 and 16 inclusive). Defaults to 10.
 * @param width   Minimum amount of space in string the conversion will use.
 * @param fill    Fill character.
 * @return  String representation of num.
 */
qcc::String AJ_CALL I32ToString(int32_t num, unsigned int base = 10, size_t width = 1, char fill = ' ');


/**
 * Convert uint64_t to a string.
 *
 * @param num     Number to convert.
 * @param base    Base (radix) for output string. (Must be between 1 and 16 inclusive). Defaults to 10.
 * @param width   Minimum amount of space in string the conversion will use.
 * @param fill    Fill character.
 * @return  String representation of num.
 */
qcc::String AJ_CALL U64ToString(uint64_t num, unsigned int base = 10, size_t width = 1, char fill = ' ');


/**
 * Convert int64_t to a string.
 *
 * @param num     Number to convert.
 * @param base    Base (radix) for output string. (Must be between 1 and 16 inclusive). Defaults to 10.
 * @param width   Minimum amount of space in string the conversion will use.
 * @param fill    Fill character.
 * @return  String representation of num.
 */
qcc::String AJ_CALL I64ToString(int64_t num, unsigned int base = 10, size_t width = 1, char fill = ' ');


/**
 * Convert decimal or hex formatted string to a uint32_t.
 *
 * @param inStr     String representation of number.
 * @param base      Base (radix) representation of inStr. 0 indicates autodetect according to C nomenclature. Defaults to 0. (Must be between 0 and 16).
 * @param badValue  Value returned if string (up to EOS or first whitespace character) is not parsable as a number.
 */
uint32_t AJ_CALL StringToU32(const qcc::String& inStr, unsigned int base = 0, uint32_t badValue = 0);


/**
 * Convert decimal or hex formatted string to an int32_t.
 *
 * @param inStr     String representation of number.
 * @param base      Base (radix) representation of inStr. 0 indicates autodetect according to C nomenclature. Defaults to 0. (Must be between 0 and 16).
 * @param badValue  Value returned if string (up to EOS or first whitespace character) is not parsable as a number.
 */
int32_t AJ_CALL StringToI32(const qcc::String& inStr, unsigned int base = 0, int32_t badValue = 0);


/**
 * Convert decimal or hex formatted string to a uint64_t.
 *
 * @param inStr     String representation of number.
 * @param base      Base (radix) representation of inStr. 0 indicates autodetect according to C nomenclature. Defaults to 0. (Must be between 0 and 16).
 * @param badValue  Value returned if string (up to EOS or first whitespace character) is not parsable as a number.
 */
uint64_t AJ_CALL StringToU64(const qcc::String& inStr, unsigned int base = 0, uint64_t badValue = 0);


/**
 * Convert decimal or hex formatted string to an int64_t.
 *
 * @param inStr     String representation of number.
 * @param base      Base (radix) representation of inStr. 0 indicates autodetect according to C nomenclature. Defaults to 0. (Must be between 0 and 16).
 * @param badValue  Value returned if string (up to EOS or first whitespace character) is not parsable as a number.
 */
int64_t AJ_CALL StringToI64(const qcc::String& inStr, unsigned int base = 0, int64_t badValue = 0);


/**
 * Convert numeric string to an double.
 *
 * @param inStr     String representation of number.
 */
double AJ_CALL StringToDouble(const qcc::String& inStr);


/**
 * Remove leading and trailing whilespace from string.
 *
 * @param inStr  Input string.
 * @return  inStr with leading and trailing whitespace removed.
 */
qcc::String AJ_CALL Trim(const qcc::String& inStr);


/**
 * Test whether character is a white space character.
 *
 * @param c            Character to test.
 * @param whiteChars   Optional Null terminated "C" string containing white chars. If not
 *                     specified, " \t\r\n" is used as the set of white space chars.
 * @return true iff c is a white space character.
 */
bool AJ_CALL IsWhite(char c, const char* whiteChars = 0);

/**
 * Format a string for output by inserting newlines into a string a regular intervals.
 *
 * @param inStr   The string to format.
 * @param maxLen  The maximum length before inserting a newline.
 * @param indent  An optional indendation
 *
 * @return  The formatted string.
 */
qcc::String AJ_CALL LineBreak(const qcc::String& inStr, size_t maxLen = 64, size_t indent = 0);

/**
 * Concatenate strings in a string vector
 *
 * @param list  A vector of strings
 * @param sep   A separator to put between the strings
 */
qcc::String AJ_CALL StringVectorToString(const std::vector<qcc::String>* list, const char* sep = "");

/**
 * Convert a character digit to its non-ascii value
 *
 * @param c            Character to convert.

 * @return non-ascii digit value or 255 if conversion was in error.
 */
uint8_t AJ_CALL CharToU8(const char c);


/**
 * Convert non-ascii digit to its character value
 *
 * @param d            Value to convert.

 * @return ascii value or '\0' if conversion was in error.
 */
char AJ_CALL U8ToChar(uint8_t d);

/**
 * Tests if character value is a base 10 digit
 *
 * @param c            Value to test.

 * @return true iff digit is valid base 10.
 */
bool AJ_CALL IsDecimalDigit(char c);

/**
 * Tests if character value is a letter of the alphabet
 *
 * @param c            Value to test.

 * @return true iff digit is valid letter
 */
bool AJ_CALL IsAlpha(char c);

/**
 * Tests if character value is alphanumeric
 *
 * @param c            Value to test.

 * @return true iff digit is an alphanumeric character
 */
bool AJ_CALL IsAlphaNumeric(char c);

/**
 * Tests if character value is a base 8 digit
 *
 * @param c            Value to test.

 * @return true iff digit is valid base 8.
 */
bool AJ_CALL IsOctalDigit(char c);

/**
 * Tests if character value is a base 16 digit
 *
 * @param c            Value to test.

 * @return true iff digit is valid base 16.
 */
bool AJ_CALL IsHexDigit(char c);

}   /* End namespace */

#endif
