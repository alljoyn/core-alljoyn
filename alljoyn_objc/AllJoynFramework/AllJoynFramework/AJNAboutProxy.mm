////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#import <alljoyn/BusAttachment.h>
#import <alljoyn/AboutProxy.h>
#import "AJNSessionOptions.h"
#import "AJNObject.h"
#import "AJNStatus.h"
#import "AJNAboutDataListener.h"
#import "AJNMessageArgument.h"
#import "AJNBusAttachment.h"
#import "AJNAboutProxy.h"

using namespace ajn;

@interface AJNBusAttachment(Private)

@property (nonatomic, readonly) BusAttachment *busAttachment;

@end

@interface AJNMessageArgument(Private)

@property (nonatomic, readonly) MsgArg *msgArg;

@end

@interface AJNAboutProxy()
/**
 * The bus attachment this object is associated with.
 */
@property (nonatomic, weak) AJNBusAttachment *bus;
@property (nonatomic, readonly) AboutProxy *aboutProxy;
@end


@implementation AJNAboutProxy

@synthesize bus = _bus;

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (AboutProxy*)aboutProxy
{
    return static_cast<AboutProxy*>(self.handle);
}
/**
 * AboutProxy Constructor
 *
 * @param  bus reference to BusAttachment
 * @param[in] busName Unique or well-known name of AllJoyn bus
 * @param[in] sessionId the session received after joining AllJoyn session
 */
- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment busName:(NSString *)serviceName sessionId:(AJNSessionId)sessionId
{
    self = [super init];
    if (self) {
        self.bus = busAttachment;
        self.handle = new AboutProxy(*((BusAttachment*)busAttachment.handle), [serviceName UTF8String], sessionId);
    }
    return self;
}

/**
 * Get the ObjectDescription array for specified bus name.
 *
 * @param[out] objectDescs  objectDescs  Description of busName's remote objects.
 *
 * @return
 *   - ER_OK if successful.
 *   - ER_BUS_REPLY_IS_ERROR_MESSAGE on unknown failure.
 */

- (QStatus)getObjectDescriptionUsingMsgArg:(AJNMessageArgument *)objectDesc
{
    return self.aboutProxy->GetObjectDescription(*objectDesc.msgArg);
}

/**
 * Get the AboutData  for specified bus name.
 *
 * @param[in] languageTag is the language used to request the AboutData.
 * @param[out] data is reference of AboutData that is filled by the function
 *
 * @return
 *    - ER_OK if successful.
 *    - ER_LANGUAGE_NOT_SUPPORTED if the language specified is not supported
 *    - ER_BUS_REPLY_IS_ERROR_MESSAGE on unknown failure
 */

- (QStatus)getAboutDataForLanguage:(NSString *)language usingDictionary:(NSMutableDictionary **)aboutData
{
    
    MsgArg msgArg;
    MsgArg* dictEntries;
    size_t numEntries;
    char *key;
    MsgArg *value;
    QStatus status = self.aboutProxy->GetAboutData([language UTF8String], msgArg);
    if (ER_OK == status) {
        msgArg.Get("a{sv}", &numEntries, &dictEntries);
        *aboutData = [[NSMutableDictionary alloc] initWithCapacity:numEntries];
        for (int i = 0 ; i < numEntries ;i++) {
            dictEntries[i].Get("{sv}", &key, &value);
            AJNMessageArgument *arg = [[AJNMessageArgument alloc] init];
            *arg.msgArg = *value;
            [arg stabilize];
            [*aboutData setValue:arg forKey:[NSString stringWithCString:key encoding:NSUTF8StringEncoding ]];
        }
    }
    return status;
}

/**
 * GetVersion get the About version
 *
 * @param[out] version of the service.
 *
 * @return ER_OK on success
 */

- (QStatus)getVersion:(uint16_t *)version
{
    return self.aboutProxy->GetVersion(*version);
}

@end