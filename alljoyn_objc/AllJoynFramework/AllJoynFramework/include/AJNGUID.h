////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
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
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNObject.h"
#import "AJNHandle.h"

/**
 * class for creating 128 bit GUIDs
 */
@interface AJNGUID128 : AJNObject

/**
 * Get the GUID raw bytes.
 *
 * @return   Pointer to GUID128::SIZE bytes that make up this guid value.
 */
@property (nonatomic, readonly) const uint8_t* bytes;

/**
 * Size of a GUID128 in bytes
 */
+ (size_t)SIZE;

/**
 * Size of string returned by ToShortString() in bytes.
 */
+ (size_t)SIZE_SHORT;

/**
 * GUID128 constructor - intializes GUID with a random number
 */
- (id)init;

/**
 * GUID128 constructor - fills GUID with specified value.
 */
- (id)initWithValue:(uint8_t)initValue;

/**
 * GUID128 constructor - intializes GUID from a hex encoded string
 */
- (id)initWithHexString:(NSString*)hexStr;

/**
 * Compare two GUIDs for equality
 */
- (BOOL)isEqual:(AJNGUID128*)toGUID;

/**
 * Compare two GUIDs for non-equality
 */
- (BOOL)isNotEqual:(AJNGUID128*)toGUID;

/**
 * Compare two GUIDs
 */
- (BOOL)isLessThan:(AJNGUID128*)toGUID;

/**
 * Compare a GUID with a string (case insensitive)
 *
 * @param other   The other GUID to compare with
 * @return True if the other string represents the same set of bytes stored
 *              in the GUID128 class.
 */
- (BOOL)compare:(NSString*)other;

/**
 * Returns string representation of a GUID128
 */
- (NSString*)description;

/**
 * Returns a shortened and compressed representation of a GUID128.
 * The result string is composed of the following characters:
 *
 *    [0-9][A-Z][a-z]-
 *
 * These 64 characters (6 bits) are stored in an 8 string. This gives
 * a 48 string that is generated uniquely from the original 128-bit GUID value.
 * The mapping of GUID128 to "shortened string" is therefore many to one.
 *
 * This representation does NOT have the full 128 bits of randomness
 *
 * @return The shortened and compressed representation of the GUID128
 */
- (NSString*)shortDescription;

/**
 * Render a GUID as an array of bytes
 */
- (uint8_t*)render:(uint8_t*)data len:(size_t)len;

/**
 * Render a GUID as a byte string.
 */
- (NSString*)renderByteString;

/**
 * Set the GUID raw bytes.
 *
 * @param buf Pointer to 16 raw (binary) bytes for guid
 */
- (void)setBytes:(const uint8_t*)buf;

/**
 * Returns true if the string is a guid or starts with a guid
 *
 * @param  str      The string to check
 *
 * @return  true if the string is a guid
 */
+ (BOOL)isGuid:(NSString*)str;

/**
 * Returns true if the string is a guid or starts with a guid
 *
 * @param str      The string to check
 * @param exactlLen  If true the string must be the exact length for a GUID128 otherwise only check
 *                  that the string starts with a GUID128
 *
 * @return  true if the string is a guid
 */
+ (BOOL)isGuid:(NSString*)str withExactLen:(BOOL)exactlLen;

/**
 * Get the GUID raw bytes wrapped in NSMutableData
 *
 * @return  GUID raw bytes
 */
- (NSMutableData *)getGuidData;

@end
