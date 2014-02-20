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
