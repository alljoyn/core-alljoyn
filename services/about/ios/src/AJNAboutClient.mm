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

#import "AJNAboutClient.h"
#import "AboutClient.h"
#import "AJNAboutDataConverter.h"
#import "AJNConvertUtil.h"

@interface AJNAboutClient ()

@property ajn::services::AboutClient *handle;

@end

@implementation AJNAboutClient

- (void)dealloc
{
	if (self.handle) {
		delete self.handle;
		self.handle = NULL;
	}
}

- (id)initWithBus:(AJNBusAttachment *)bus
{
	self = [super init];
	if (self) {
		self.handle = new ajn::services::AboutClient((ajn::BusAttachment&)(*bus.handle));
	}
	return self;
}

- (QStatus)objectDescriptionsWithBusName:(NSString *)busName
                   andObjectDescriptions:(NSMutableDictionary **)objectDescs
                            andSessionId:(uint32_t)sessionId
{
	ajn::services::AboutClient::ObjectDescriptions objectDescriptions;
	QStatus status = self.handle->GetObjectDescriptions([AJNConvertUtil convertNSStringToConstChar:busName], objectDescriptions, (ajn::SessionId)sessionId);
	*objectDescs = [AJNAboutDataConverter convertToObjectDescriptionsDictionary:objectDescriptions];
    
	return status;
}

- (QStatus)aboutDataWithBusName:(NSString *)busName
                 andLanguageTag:(NSString *)languageTag
                   andAboutData:(NSMutableDictionary **)data
                   andSessionId:(uint32_t)sessionId
{
	ajn::services::AboutClient::AboutData aboutData;
    
	QStatus status =  self.handle->GetAboutData([AJNConvertUtil convertNSStringToConstChar:busName], [AJNConvertUtil convertNSStringToConstChar:languageTag], aboutData, (ajn::SessionId)sessionId);
	*data = [AJNAboutDataConverter convertToAboutDataDictionary:aboutData];
	return status;
}

- (QStatus)versionWithBusName:(NSString *)busName
                   andVersion:(int)version
                 andSessionId:(AJNSessionId)sessionId
{
	return (self.handle->GetVersion([AJNConvertUtil convertNSStringToConstChar:busName], version, (ajn::SessionId)sessionId));
}

@end
