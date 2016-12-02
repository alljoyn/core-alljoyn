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

#import "AJNAboutServiceApi.h"
#import "AboutServiceApi.h"
#import "PropertyStore.h"
#import "AJNPropertyStore.h"

@interface AJNAboutServiceApi ()
@property (nonatomic) ajn::services::AboutPropertyStoreImpl *handle;

- (id)init;

@end

@implementation AJNAboutServiceApi

// Destroy the instance after finished
- (void)destroyInstance
{
	[super unregister];
	ajn::services::AboutServiceApi::DestroyInstance();
	if (self.handle) {
		delete self.handle;
		self.handle = NULL;
	}
}

+ (id)sharedInstance
{
	static AJNAboutServiceApi *ajnAboutServiceApi;
	static dispatch_once_t donce;
	dispatch_once(&donce, ^{
	    ajnAboutServiceApi = [[self alloc] init];
	});
	return ajnAboutServiceApi;
}

- (id)init
{
	if (self = [super init]) {
	}
	return self;
}

- (void)startWithBus:(AJNBusAttachment *)bus
    andPropertyStore:(AJNAboutPropertyStoreImpl *)store
{
	if (self.isServiceStarted) {
         NSLog(@"[%@] [%@] Service already started", @"DEBUG", [[self class] description]);

		return;
	}
	self.handle =  [store getHandle];
	[super registerBus:bus andPropertystore:store];
	ajn::services::AboutServiceApi::Init(*(ajn::BusAttachment *)bus.handle, *self.handle);
	ajn::services::AboutServiceApi::getInstance();
}

- (ajn::services::AboutPropertyStoreImpl *)getPropertyStore
{
	return self.handle;
}

@end