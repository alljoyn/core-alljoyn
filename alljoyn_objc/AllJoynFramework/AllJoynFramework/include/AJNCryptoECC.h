////////////////////////////////////////////////////////////////////////////////
// //    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
//    Source Project (AJOSP) Contributors and others.
//
//    SPDX-License-Identifier: Apache-2.0
//
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//     PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import <alljoyn/Status.h>
#import "AJNObject.h"

/**
 * The ECC public key
 *
 * At the moment, because the code only supports one curve, public keys
 * are not innately tied to a particular curve. In the future, if the code
 * supports more than one curve, a public key should store its curve also.
 */
@interface AJNECCPublicKey : AJNObject

/**
 * Check to see if the ECCPublicKey is empty
 *
 * @return true if the ECCPublicKey is empty
 */
@property (nonatomic, readonly) BOOL isEmpty;

/**
 * Return the size of the public key in exported form
 *
 * @return Size of the exported public key
 */
@property (nonatomic, readonly) size_t size;

- (id)init;

/**
 * Clear the key to make it empty.
 */
- (void)clear;

/**
 * Equals operator
 *
 * @param[in] toKey the ECCPublic key to compare
 *
 * @return true if the compared ECCPublicKeys are equal to each other
 */
- (BOOL)isEqualTo:(AJNECCPublicKey*)toKey;

/**
 * Not equals operator
 *
 * @param[in] toKey the ECCPublicKey to compare
 *
 * @return true if the compared ECCPublicKeys are not equal to each other
 */
- (BOOL)isNotEqualTo:(AJNECCPublicKey*)toKey;

/**
 * The less than operator for the ECCPublicKey
 *
 * The x coordinate are compared first. If the x coordinates match then
 * the y coordinate is compared.
 *
 * @param[in] otherKey The ECCPublicKey to compare
 *
 * @return True if the AJNECCPublicKey is less than otherKey;
 * false otherwise.
 */
- (BOOL)isLessThan:(AJNECCPublicKey*)otherKey;

/**
 * Export the key to a byte array. The X and Y coordinates are concatenated in that order, and each
 * occupy exactly half of the returned array. The X coordinate is in the first half, and the Y coordinate
 * in the second. Use the returned size divided by two as the length of an individual coordinate.
 * @param[in] data the array to store the data in
 * @param[in,out] size provides the size of the passed buffer as input. On a successful return it
 *   will contain the actual amount of data stored
 *
 * @return ER_OK on success others on failure
 */
- (QStatus)export:(uint8_t*)data size:(size_t*)size;

/**
 * Import the key from a byte array
 * @param[in] data the array to store the data in
 * @param[in] size the size of the passed buffer
 *
 * @return ER_OK  on success others on failure
 */
- (QStatus)import:(uint8_t*)data size:(size_t)size;

/**
 * Import the key from two byte arrays, one containing each coordinate
 * @param[in] xData array containing the bytes of the X coordinate
 * @param[in] xSize length of xData
 * @param[in] yData array containing the bytes of the Y coordinate
 * @param[in] ySize length of yData
 *
 * @return ER_OK   on success others on failure
 */
- (QStatus)import:(uint8_t*)xData xSize:(size_t)xSize yData:(uint8_t*)yData ySize:(size_t)ySize;

/**
 * Return the ECCPublicKey to a string
 * @return the ECCPublicKey as a string.
 */
- (NSString*)description;

@end

@interface AJNECCPrivateKey : AJNObject

/*
 * Construct an AJNECCPrivateKey
 */
- (id)init;

/**
 * Get the size of the private key value
 *
 * @return Size of the private key in bytes
 */
@property (nonatomic, readonly) size_t size;

/**
 * Import the key from a byte array.
 * @param[in] data the array to store the data in
 *
 * @return ER_OK  on success others on failure
 */
- (QStatus)import:(NSData*)data;

/**
 * Export the key to a byte array.
 * @param[in] data the array to store the data in
 *
 * On a successful return it will contain the actual amount of data stored, which is the same value
 * as returned by GetSize(). On ER_BUFFER_TOO_SMALL, contains the amount of storage required, which
 * is also the value returned by GetSize().
 *
 * @return ER_OK on success others on failure
 */
- (QStatus)export:(NSMutableData*)data;

/**
 * Equals operator for the ECCPrivateKey.
 *
 * @param[in] otherKey the ECCPrivateKey to compare
 *
 * @return true if the ECCPrivateKeys are equal
 */
- (BOOL)isEqualTo:(AJNECCPrivateKey*)otherKey;

/**
 * Return the ECCPrivateKey as a string
 * @return the ECCPrivateKey as a string
 */
- (NSString*)description;

@end

@interface AJNECCSignature : AJNObject

/*
 * Construct an AJNECCSignature
 */
- (id)init;

@end