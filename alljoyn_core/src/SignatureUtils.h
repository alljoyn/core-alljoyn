#ifndef _SIGNATURE_UTILS_H
#define _SIGNATURE_UTILS_H
/**
 * @file
 * This file defines a set of utilities functions for processing AllJoyn signatures.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#ifndef __cplusplus
#error Only include SignatureUtils.h in C++ code.
#endif

#include <qcc/platform.h>

#include <alljoyn/Status.h>


namespace ajn {

class SignatureUtils {

  public:

    /**
     * Indicate if a type id is a basic type, other types are containers. Keys in dictionary
     * entries must be basic types.
     *
     * @param typeId  The type id to test
     *
     * @return
     *      - True if the type is a basic type
     *      - False otherwise
     */
    static bool IsBasicType(AllJoynTypeId typeId);

    /**
     * Returns the byte alignment for a specified typeId.
     *
     * @param typeId   The type for which the alignment is required.
     *
     * @return  The byte aligmnent for the type.
     */
    static size_t AlignmentForType(AllJoynTypeId typeId);

    /**
     * Builds a signature for an array of message arg values.
     *
     * @param values     A pointer to an array of message arg values
     * @param numValues  Length of the array
     * @param sig        Buffer to receive the signature must be at least 256 bytes long
     * @param len        Returns the length of the signature
     * @remark The maximum length of the signature is 255.
     * @return
     *      - ER_OK if the signature was built.
     *      - An error status otherwise.
     */
    static QStatus MakeSignature(const MsgArg* values, uint8_t numValues, char* sig, size_t& len);

    /**
     * Compute the size of an array of MsgArg values taking alignment requirements into account
     *
     * @param values     A pointer to an array of data values
     * @param numValues  Length of the array
     * @param sz         The starting offset so alignment can be correctly computed.
     *
     * @return  The marshaled size of the array of MsgArgs
     */
    static size_t GetSize(const MsgArg* values, size_t numValues, size_t offset = 0);

    /**
     * Parses a complete type leaving the signature pointer pointing at the first character after
     * the complete type.
     *
     * @param sigPtr  Pointer to the signature to parse
     *
     * @return
     *      - ER_OK if the signature was prefixed by a complete type.
     *      - ER_BUS_BAD_SIGNATURE  if unable to parse the complete type.
     */
    static QStatus ParseCompleteType(const char*& sigPtr);

    /**
     * Check that a string is a valid signature. Valid signature is no longer than 255 character and
     * contains 0 or more complete types.
     *
     * @param signature  The signature to check.
     *
     * @return Returns true if the signature is valid, false otherwise.
     */
    static bool IsValidSignature(const char* signature);

    /**
     * Counts the number of complete types in a signature
     *
     * @param signature  The signature
     *
     * @return   The number of complete types in the signature
     */
    static uint8_t CountCompleteTypes(const char* signature);

    /**
     * Check if a signature is a single complete type.
     *
     * @param signature  The signature to check.
     *
     * @return  Returns true if the signature is valid and is one complete type.
     */
    static bool IsCompleteType(const char* signature) { return (ParseCompleteType(signature) == ER_OK) && (*signature == 0); }

    /**
     * Parses and verifies a signature for a container type
     *
     * @param container  The container to parse.
     * @param sigPtr     Points the first character after the @c 'a', @c '(', or @c '{'
     * @return
     *  - ER_OK if the signature for a container type was verified.
     *  - ER_BUS_BAD_SIGNATURE  if unable to parse signature for the container type.
     */
    static QStatus ParseContainerSignature(MsgArg& container, const char*& sigPtr);

};

}

#endif
