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

#ifndef AJNAboutService_PropertyStore_h
#define AJNAboutService_PropertyStore_h

#import <Foundation/Foundation.h>
#import "alljoyn/Status.h"

/**
 AJNPropertyStore protocol overview
 */
@protocol AJNPropertyStore <NSObject>

#pragma mark – Protocol enum
///---------------------
/// @name Protocol enum
///---------------------
/**
 * Filter has three possible values ANNOUNCE, READ,WRITE
 * READ is for data that is marked as read
 * ANNOUNCE is for data that is marked as announce
 * WRITE is for data that is marked as write
 */
typedef enum {
	ANNOUNCE,     //!< ANNOUNCE Property that has  ANNOUNCE  enabled
	READ,        //!< READ     Property that has READ  enabled
	WRITE,       //!< WRITE    Property that has  WRITE  enabled
} PFilter;


#define QAS_ER_LANGUAGE_NOT_SUPPORTED       ((QStatus)0xb001)
#define QAS_ER_FEATURE_NOT_AVAILABLE        ((QStatus)0xb002)
#define QAS_ER_INVALID_VALUE                        ((QStatus)0xb003)
#define QAS_ER_MAX_SIZE_EXCEEDED            ((QStatus)0xb004)

#pragma mark – Protocol methods

/**
 Calls Reset() implemented only for  ConfigService
 @return status
 */
- (QStatus)reset;

/**
 Update properties method
 @param name name of the property
 @param languageTag languageTag is the language to use for the action can be NULL meaning default.
 @param value value is a pointer to the data to change.
 @return status
 */
- (QStatus)updatePropertyName:(NSString *)name andLanguageTag:(NSString *)languageTag andValue:(NSMutableArray **)value;

/**
 Delete property method
 @param name name of the property
 @param languageTag languageTag is the language to use for the action can't be NULL.
 @return status
 */
- (QStatus)deletePropertyName:(NSString *)name andLanguageTag:(NSString *)languageTag;

@end

#endif //  AJNAboutService_PropertyStore_h
