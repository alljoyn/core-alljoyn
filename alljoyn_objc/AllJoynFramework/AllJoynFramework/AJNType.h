////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>

/**
 * Enumeration of the various message arg types.
 * @remark Most of these map directly to the values used in the
 * DBus wire protocol but some are specific to the AllJoyn implementation.
 */
typedef enum {
    kAJNTypeInvalid          =  0,     ///< AllJoyn INVALID typeId
    kAJNTypeArray            = 'a',    ///< AllJoyn array container type
    kAJNTypeBoolean          = 'b',    ///< AllJoyn boolean basic type, @c 0 is @c FALSE and @c 1 is @c TRUE - Everything else is invalid
    kAJNTypeDouble           = 'd',    ///< AllJoyn IEEE 754 double basic type
    kAJNTypeDictionaryEntry  = 'e',    ///< AllJoyn dictionary or map container type - an array of key-value pairs
    kAJNTypeSignature        = 'g',    ///< AllJoyn signature basic type
    kAJNTypeHandle           = 'h',    ///< AllJoyn socket handle basic type
    kAJNTypeInt32            = 'i',    ///< AllJoyn 32-bit signed integer basic type
    kAJNTypeInt16            = 'n',    ///< AllJoyn 16-bit signed integer basic type
    kAJNTypeObjectPath       = 'o',    ///< AllJoyn Name of an AllJoyn object instance basic type
    kAJNTypeUInt16           = 'q',    ///< AllJoyn 16-bit unsigned integer basic type
    kAJNTypeStruct           = 'r',    ///< AllJoyn struct container type
    kAJNTypeString           = 's',    ///< AllJoyn UTF-8 NULL terminated string basic type
    kAJNTypeUInt64           = 't',    ///< AllJoyn 64-bit unsigned integer basic type
    kAJNTypeUInt32           = 'u',    ///< AllJoyn 32-bit unsigned integer basic type
    kAJNTypeVariant          = 'v',    ///< AllJoyn variant container type
    kAJNTypeInt64            = 'x',    ///< AllJoyn 64-bit signed integer basic type
    kAJNTypeByte             = 'y',    ///< AllJoyn 8-bit unsigned integer basic type

    kAJNTypeStructOpen       = '(', /**< Never actually used as a typeId: specified as ALLJOYN_STRUCT */
    kAJNTypeStructClose      = ')', /**< Never actually used as a typeId: specified as ALLJOYN_STRUCT */
    kAJNTypeDictEntryOpen    = '{', /**< Never actually used as a typeId: specified as ALLJOYN_DICT_ENTRY */
    kAJNTypeDictEntryClose   = '}', /**< Never actually used as a typeId: specified as ALLJOYN_DICT_ENTRY */

    kAJNTypeBooleanArray     = ('b' << 8) | 'a',   ///< AllJoyn array of booleans
    kAJNTypeDoubleArray      = ('d' << 8) | 'a',   ///< AllJoyn array of IEEE 754 doubles
    kAJNTypeInt32Array       = ('i' << 8) | 'a',   ///< AllJoyn array of 32-bit signed integers
    kAJNTypeInt16Array       = ('n' << 8) | 'a',   ///< AllJoyn array of 16-bit signed integers
    kAJNTypeUInt16Array      = ('q' << 8) | 'a',   ///< AllJoyn array of 16-bit unsigned integers
    kAJNTypeUInt64Array      = ('t' << 8) | 'a',   ///< AllJoyn array of 64-bit unsigned integers
    kAJNTypeUInt32Array      = ('u' << 8) | 'a',   ///< AllJoyn array of 32-bit unsigned integers
    kAJNTypeInt64Array       = ('x' << 8) | 'a',   ///< AllJoyn array of 64-bit signed integers
    kAJNTypeByteArray        = ('y' << 8) | 'a',   ///< AllJoyn array of 8-bit unsigned integers

    kAJNTypeWildcard         = '*'     ///< This never appears in a signature but is used for matching arbitrary message args

} AJNType;
