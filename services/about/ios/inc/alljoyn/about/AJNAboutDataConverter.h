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
#import "MsgArg.h"
#import "AJNMessageArgument.h"
#import "alljoyn/about/AnnounceHandler.h"
#import "alljoyn/about/AJNAboutClient.h"

/**
 AJNAboutDataConverter is a utility class to convert About Service data structurs into an Objective-c objects.
 Note that incase of illegal content - an emty string will be in use:  ERRORSTRING = @"";
 */
@interface AJNAboutDataConverter : NSObject

/**
 Convert AJNMessageArgument to NSString.
 @param ajnMsgArg An AJNMessageArgument object.
 @return NSString representation of the AJNMessageArgument content.
 */
+ (NSString *)messageArgumentToString:(AJNMessageArgument *)ajnMsgArg;

/**
 Convert NSMutableDictionary of about data in the format of NSString/AJNMessageArgument to NSString.
 @param aboutDataDict NSMutableDictionary of about data in the format of NSString/AJNMessageArgument.
 @return NSString representation of the NSMutableDictionary content.
 */
+ (NSString *)aboutDataDictionaryToString:(NSMutableDictionary *)aboutDataDict;

/**
 Convert c++ map of about data in the format of qcc::String/ajn::MsgArg to NSMutableDictionary of NSString/AJNMessageArgument.
 @param aboutData C++ map of about data in the format of qcc::String/ajn::MsgArg.
 @return NSMutableDictionary of NSString/AJNMessageArgument.
 */
+ (NSMutableDictionary *)convertToAboutDataDictionary:(const ajn ::services ::AnnounceHandler ::AboutData&)aboutData;

/** Convert c++ map of object descriptions in the format of qcc::String/std::vector<qcc::String> to NSMutableDictionary of NSString/NSMutableArray.
 @param objectDescs C++ map of object descriptions in the format of qcc::String/std::vector<qcc::String>.
 @return NSMutableDictionary of NSString/NSMutableArray.
 */
+ (NSMutableDictionary *)convertToObjectDescriptionsDictionary:(const ajn ::services ::AnnounceHandler ::ObjectDescriptions&)objectDescs;

/**
 Convert NSMutableDictionary of object descriptions in the format of NSString/NSMutableArray to NSString.
 @param objectDescDict NSMutableDictionary of object descriptions in the format of NSString/NSMutableArray.
 @return NSString representation of the object descriptions NSMutableDictionary content.
 */
+ (NSString *)objectDescriptionsDictionaryToString:(NSMutableDictionary *)objectDescDict;
@end
