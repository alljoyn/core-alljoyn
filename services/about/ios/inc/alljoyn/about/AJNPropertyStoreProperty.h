/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#import <Foundation/Foundation.h>
#import "alljoyn/about/PropertyStoreProperty.h"
@class AJNMessageArgument;

/**
 AJNPropertyStoreProperty is a wrapper of AJNMessageArgument for use in the property store
 */

@interface AJNPropertyStoreProperty : NSObject

@property (strong, nonatomic) NSString *keyName;
@property (strong, nonatomic) AJNMessageArgument *value;
@property (strong, nonatomic) NSString *language;
@property (nonatomic) bool isPublic;
@property (nonatomic) bool isWritable;
@property (nonatomic) bool isAnnouncable;

/**
 Designated initializer using a pointer to a C++ property store property instance
 @param handle handle to the instance
 */

- (id)initWithHandle:(ajn ::services ::PropertyStoreProperty *)handle;

/**
 Designated initializer which inits a new object for a key
 @param keyName the key to use with the entry to the property store
 */
- (id)initPropertyStorePropertyWithKey:(NSString *)keyName;

/**
 Designated initializer which inits a new object for a key and adds a value
 @param keyName the key to use with the entry to the property store
 @param value the value to hold
 */
- (id)initPropertyStorePropertyWithKey:(NSString *)keyName value:(AJNMessageArgument *)value;

/**
 Designated initializer which inits a new object
 @param keyName the key to use with the entry to the property store
 @param value the value to hold
 @param isPublic is this property public?
 @param isWritable is this property writable? (if true it will be sent as a configurable entry to the client)
 @param isAnnouncable is this property part of the announce notification?
 */
- (id)initPropertyStorePropertyWithKey:(NSString *)keyName value:(AJNMessageArgument *)value isPublic:(bool)isPublic isWritable:(bool)isWritable isAnnouncable:(bool)isAnnouncable;

/**
 Designated initializer which inits a new object like in the above for a specific language
 @param keyName the key to use with the entry to the property store
 @param value the value to hold
 @param language define this property for a specific language
 @param isPublic is this property public?
 @param isWritable is this property writable? (if true it will be sent as a configurable entry to the client)
 @param isAnnouncable is this property part of the announce notification?
 */
- (id)initPropertyStorePropertyWithKey:(NSString *)keyName value:(AJNMessageArgument *)value language:
(NSString *)language      isPublic:(bool)isPublic isWritable:(bool)isWritable isAnnouncable:(bool)isAnnouncable;


/**
 Destructor for PropertyStoreProperty
 */
- (void)dealloc;

/**
 Set the flags for the property
 @param isPublic is this property public?
 @param isWritable is this property writable? (if true it will be sent as a configurable entry to the client)
 @param isAnnouncable is this property part of the announce notification?
 */
- (void)setFlagsIsPublic:(bool)isPublic isWritable:(bool)isWritable isAnnouncable:(bool)isAnnouncable;

@end
