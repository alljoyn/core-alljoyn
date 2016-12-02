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

#import "AJNAnnouncement.h"

@interface AJNAnnouncement ()

@end

@implementation AJNAnnouncement

- (id)initWithVersion:(uint16_t)version port:(uint16_t)port
              busName:(NSString *)busName
   objectDescriptions:(NSMutableDictionary *)objectDescs
            aboutData:(NSMutableDictionary **)aboutData
{
	self = [super init];
	if (self) {
		self.version = version;
		self.port = port;
		self.busName = busName;
		self.objectDescriptions = objectDescs;
		self.aboutData = *aboutData;
	}
	return self;
}

@end