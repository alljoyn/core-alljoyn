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

#import "AJNAboutIconService.h"
#import "AboutIconService.h"
#import "AJNConvertUtil.h"

@interface AJNAboutIconService ()
@property ajn::services::AboutIconService *aboutIconServiceHandler;
@end

@implementation AJNAboutIconService

- (void)dealloc
{
	if (self.aboutIconServiceHandler) {
		delete self.aboutIconServiceHandler;
		self.aboutIconServiceHandler = NULL;
	}
}

- (id)initWithBus:(AJNBusAttachment *)bus
         mimeType:(NSString *)mimetype
              url:(NSString *)url
          content:(uint8_t *)content
            csize:(size_t)csize
{
	self = [super init];
	if (self) {
		self.aboutIconServiceHandler = new ajn::services::AboutIconService((ajn::BusAttachment&)(*bus.handle), [AJNConvertUtil convertNSStringToQCCString:(mimetype)], [AJNConvertUtil convertNSStringToQCCString:(url)], content, csize);
		//  RegisterAboutIconService calls Register(RegisterBusObject) which uses self.handle
		self.handle = self.aboutIconServiceHandler;
	}
	return self;
}

- (QStatus)registerAboutIconService
{
	return(self.aboutIconServiceHandler->Register());
}

@end