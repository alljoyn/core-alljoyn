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

#import "QASAnnouncementReceiver.h"
#import "BusAttachment.h"
#import "AnnounceHandlerAdapter.h"
#import "AnnouncementRegistrar.h"

@interface QASAnnouncementReceiver ()

@property id <QASAnnouncementListener> qasAnnouncementListener;
@property AnnounceHandlerAdapter *announceHandlerAdapter;
@property ajn::BusAttachment *busAttachment;

@end

@implementation QASAnnouncementReceiver

- (void)dealloc
{
	if (self.announceHandlerAdapter) {
		delete self.announceHandlerAdapter;
		self.announceHandlerAdapter = NULL;
	}
}

- (id)initWithAnnouncementListener:(id <QASAnnouncementListener> )announcementListener
                            andBus:(AJNBusAttachment *)bus
{
	// Create announceHandlerAdapter and save a reference
	self.announceHandlerAdapter = new AnnounceHandlerAdapter(announcementListener);
	// Save a reference to the BusAttachment
	self.busAttachment = (ajn::BusAttachment *)bus.handle;
    
	return self;
}

- (QStatus)registerAnnouncementReceiver
{
    NSLog(@"[%@] [%@] Calling %@", @"DEBUG", [[self class] description],NSStringFromSelector(_cmd));

	return ajn::services::AnnouncementRegistrar::RegisterAnnounceHandler((*(self.busAttachment)), *(self.announceHandlerAdapter));
}

- (QStatus)unRegisterAnnouncementReceiver
{
    NSLog(@"[%@] [%@] Calling %@", @"DEBUG", [[self class] description],NSStringFromSelector(_cmd));
	return ajn::services::AnnouncementRegistrar::UnRegisterAnnounceHandler(*(self.busAttachment), *(self.announceHandlerAdapter));
}

@end
