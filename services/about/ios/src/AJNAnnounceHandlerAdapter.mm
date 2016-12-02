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

#import "AJNAnnounceHandlerAdapter.h"
#import "AJNConvertUtil.h"
#import "AJNAboutDataConverter.h"

AJNAnnounceHandlerAdapter::AJNAnnounceHandlerAdapter(id <AJNAnnouncementListener> announcementListener)
{
	AJNAnnouncementListener = announcementListener;
}

AJNAnnounceHandlerAdapter::~AJNAnnounceHandlerAdapter()
{
}

void AJNAnnounceHandlerAdapter::Announce(uint16_t version, uint16_t port, const char *busName, const ObjectDescriptions& objectDescs, const AboutData& aboutData)
{
	NSMutableDictionary *ajnAboutData;
    
    NSLog(@"[%@] [%@] Received an announcemet from %s ", @"DEBUG", @"AnnounceHandlerAdapter", busName);
        
	// Convert AboutData to QASAboutData
	ajnAboutData = [AJNAboutDataConverter convertToAboutDataDictionary:aboutData];
    
	[AJNAnnouncementListener announceWithVersion:version port:port busName:[AJNConvertUtil convertConstCharToNSString:busName] objectDescriptions:[AJNAboutDataConverter convertToObjectDescriptionsDictionary:objectDescs] aboutData:&ajnAboutData];
}