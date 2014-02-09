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

#import "AJNAboutService.h"
#import "AJNSessionOptions.h"
#import "TransportMask.h"
#import "AboutService.h"
#import "AboutServiceApi.h"
#import "AJNConvertUtil.h"

@interface AJNAboutService ()
@property (nonatomic) ajn::services::AboutService *handle;
@property (nonatomic) ajn::BusAttachment *k_bus;
@property (nonatomic) id <AJNPropertyStore> k_store;
@property (nonatomic) std::vector <qcc::String> *stdVect;

@end

@implementation AJNAboutService



- (id)init
{
	if (self = [super init]) {
	}
	return self;
}

- (QStatus)registerPort:(AJNSessionPort)port
{
	if (!self.isServiceStarted) {
         NSLog(@"[%@] [%@] Service has not started", @"DEBUG", [[self class] description]);

		return ER_FAIL;
	}
	QStatus status;
	status = ajn::services::AboutServiceApi::getInstance()->Register(port);
	if (status != ER_OK) {
		 NSLog(@"[%@] [%@] Failed to Register port", @"DEBUG", [[self class] description]);

		return status;
	}
    
	if (!self.k_bus) {
        
        NSLog(@"[%@] [%@] Can't RegisterBusObject - bus Object is missing", @"DEBUG", [[self class] description]);

		return ER_FAIL;
	}
    
	status = self.k_bus->RegisterBusObject(*ajn::services::AboutServiceApi::getInstance());
	if (status != ER_OK) {
         NSLog(@"[%@] [%@] Failed to RegisterBusObject", @"DEBUG", [[self class] description]);
    }
	return status;
}

- (void)registerBus:(AJNBusAttachment *)bus
   andPropertystore:(id <AJNPropertyStore> )store
{
	self.k_bus = (ajn::BusAttachment *)bus.handle;
	self.k_store = store;
	self.isServiceStarted = true;
}

- (void)unregister
{
	if (!self.isServiceStarted) {
         NSLog(@"[%@] [%@] Service has not started", @"DEBUG", [[self class] description]);

		return;
	}
    
	ajn::services::AboutServiceApi::getInstance()->Unregister();
	self.isServiceStarted = false;
    
	self.k_bus = nil;
	self.k_store = nil;
	delete self.stdVect;
	self.stdVect = NULL;
    
	self.handle = NULL;
}

- (QStatus)addObjectDescriptionWithPath:(NSString *)path
                      andInterfaceNames:(NSMutableArray *)interfaceNames
{
	if (!self.isServiceStarted) {
         NSLog(@"[%@] [%@] Service has not started", @"DEBUG", [[self class] description]);

		return ER_FAIL;
	}
    
	std::vector <qcc::String> *tvect = [self convertNSMutableArrayToStdVector:(interfaceNames)];
	return (ajn::services::AboutServiceApi::getInstance()->AddObjectDescription([AJNConvertUtil convertNSStringToConstChar:(path)], *tvect));
}

- (QStatus)removeObjectDescriptionWithPath:(NSString *)path
                         andInterfaceNames:(NSMutableArray *)interfaceNames
{
	if (!self.isServiceStarted) {
         NSLog(@"[%@] [%@] Service has not started", @"DEBUG", [[self class] description]);
        
        return ER_FAIL;
	}
	return (ajn::services::AboutServiceApi::getInstance()->RemoveObjectDescription([AJNConvertUtil convertNSStringToConstChar:(path)], *[self convertNSMutableArrayToStdVector:(interfaceNames)]));
}

- (QStatus)announce
{
	if (!self.isServiceStarted) {
         NSLog(@"[%@] [%@] Service has not started", @"DEBUG", [[self class] description]);

        return ER_FAIL;
	}
    
	return(ajn::services::AboutServiceApi::getInstance()->Announce());
}

#pragma mark - util methods
- (std::vector <qcc::String> *)convertNSMutableArrayToStdVector:(NSMutableArray *)mutableArr
{
	if (!self.stdVect) {
		self.stdVect = new std::vector <qcc::String>;
	}
	//  Convert NSMutableArray to std::vector<qcc::String>
	for (NSString *tStr in mutableArr) {
		self.stdVect->push_back([AJNConvertUtil convertNSStringToQCCString:(tStr)]);
	}
	return self.stdVect;
}

@end
