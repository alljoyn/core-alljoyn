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

#import "AJNAboutIconClient.h"
#import "AboutIconClient.h"
#import "AJNConvertUtil.h"

static int SESSIONID=0;

@interface AJNAboutIconClient ()
@property ajn::services::AboutIconClient *handle;

@end
@implementation AJNAboutIconClient

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
		self.handle = new ajn::services::AboutIconClient((ajn::BusAttachment&)(*bus.handle));
	}
	return self;
}

- (QStatus)urlFromBusName:(NSString *)busName
                      url:(NSString **)url
{
	return [self urlFromBusName:busName url:url sessionId:SESSIONID];
}

- (QStatus)urlFromBusName:(NSString *)busName
                      url:(NSString **)url
                sessionId:(AJNSessionId)sessionId
{
	QStatus status;
	const char *tBus = const_cast <char *>([AJNConvertUtil convertNSStringToConstChar:busName]);
	qcc::String tUrl;
    
	status = self.handle->GetUrl(tBus, tUrl, sessionId);
	// Convert the url
	*url = [AJNConvertUtil convertQCCStringtoNSString:tUrl];
	return status;
}

- (QStatus)contentFromBusName:(NSString *)busName
                      content:(uint8_t **)content
                  contentSize:(size_t&)contentSize
{
	return [self contentFromBusName:busName content:content contentSize:contentSize sessionId:SESSIONID];
}

- (QStatus)contentFromBusName:(NSString *)busName
                      content:(uint8_t **)content
                  contentSize:(size_t&)contentSize
                    sessionId:(AJNSessionId)sessionId
{
	const char *tBus = const_cast <char *>([AJNConvertUtil convertNSStringToConstChar:busName]);
	return self.handle->GetContent(tBus, content, contentSize, sessionId);
}

- (QStatus)versionFromBusName:(NSString *)busName
                      version:(int&)version
{
	return [self versionFromBusName:busName version:version sessionId:SESSIONID];
}

- (QStatus)versionFromBusName:(NSString *)busName
                      version:(int&)version
                    sessionId:(AJNSessionId)sessionId
{
	const char *tBus = const_cast <char *>([AJNConvertUtil convertNSStringToConstChar:busName]);
	return self.handle->GetVersion(tBus, version, sessionId);
}

- (QStatus)mimeTypeFromBusName:(NSString *)busName
                      mimeType:(NSString **)mimeType
{
	return [self mimeTypeFromBusName:busName mimeType:mimeType sessionId:SESSIONID];
}

- (QStatus)mimeTypeFromBusName:(NSString *)busName
                      mimeType:(NSString **)mimeType
                     sessionId:(AJNSessionId)sessionId
{
	QStatus status;
	const char *tBus = const_cast <char *>([AJNConvertUtil convertNSStringToConstChar:busName]);
	qcc::String tMimeType;
    
	status = self.handle->GetMimeType(tBus, tMimeType, sessionId);
    
	*mimeType = [AJNConvertUtil convertQCCStringtoNSString:tMimeType];
    
	return status;
}

- (QStatus)sizeFromBusName:(NSString *)busName
                      size:(size_t&)size
{
	return [self sizeFromBusName:busName size:size sessionId:SESSIONID];
}

- (QStatus)sizeFromBusName:(NSString *)busName
                      size:(size_t&)size
                 sessionId:(AJNSessionId)sessionId
{
	const char *tBus = const_cast <char *>([AJNConvertUtil convertNSStringToConstChar:busName]);
	return self.handle->GetSize(tBus, size, sessionId);
}

@end
