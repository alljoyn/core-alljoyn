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

#import "AJNAnnouncementReceiver.h"
#import "BusAttachment.h"
#import "AJNAnnounceHandlerAdapter.h"
#import "AnnouncementRegistrar.h"

@interface AJNAnnouncementReceiver ()

@property id <AJNAnnouncementListener> AJNAnnouncementListener;
@property AJNAnnounceHandlerAdapter *announceHandlerAdapter;
@property ajn::BusAttachment *busAttachment;

@end

@implementation AJNAnnouncementReceiver

- (void)dealloc
{
	if (self.announceHandlerAdapter) {
		delete self.announceHandlerAdapter;
		self.announceHandlerAdapter = NULL;
	}
}

- (id)initWithAnnouncementListener:(id <AJNAnnouncementListener> )announcementListener
                            andBus:(AJNBusAttachment *)bus
{
	// Create announceHandlerAdapter and save a reference
	self.announceHandlerAdapter = new AJNAnnounceHandlerAdapter(announcementListener);
	// Save a reference to the BusAttachment
	self.busAttachment = (ajn::BusAttachment *)bus.handle;
    
	return self;
}

- (QStatus)registerAnnouncementReceiverForInterfaces:(const char  **)interfaces withNumberOfInterfaces:(size_t)numOfInterfaces
{
    NSLog(@"[%@] [%@] Calling %@", @"DEBUG", [[self class] description],NSStringFromSelector(_cmd));
    return ajn::services::AnnouncementRegistrar::RegisterAnnounceHandler((*(self.busAttachment)), *(self.announceHandlerAdapter), interfaces, numOfInterfaces);
}

- (QStatus)registerAnnouncementReceiver
{
    NSLog(@"[%@] [%@] Calling %@", @"DEBUG", [[self class] description],NSStringFromSelector(_cmd));
    return ajn::services::AnnouncementRegistrar::RegisterAnnounceHandler((*(self.busAttachment)), *(self.announceHandlerAdapter), NULL, 0);
}

- (QStatus)unRegisterAnnouncementReceiverForInterfaces:(const char  **)interfaces withNumberOfInterfaces:(size_t)numOfInterfaces
{
    NSLog(@"[%@] [%@] Calling %@", @"DEBUG", [[self class] description],NSStringFromSelector(_cmd));
	return ajn::services::AnnouncementRegistrar::UnRegisterAnnounceHandler(*(self.busAttachment), *(self.announceHandlerAdapter), interfaces, numOfInterfaces);
}

- (QStatus)unRegisterAnnouncementReceiver
{
    NSLog(@"[%@] [%@] Calling %@", @"DEBUG", [[self class] description],NSStringFromSelector(_cmd));
    return ajn::services::AnnouncementRegistrar::UnRegisterAnnounceHandler(*(self.busAttachment), *(self.announceHandlerAdapter), NULL, 0);
}

@end