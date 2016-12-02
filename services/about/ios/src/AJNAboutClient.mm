/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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