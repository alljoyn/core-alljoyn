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

#import <Foundation/Foundation.h>

/**
 AJNAnnouncementListener is a helper protocol used by AnnounceHandlerAdapter to receive AboutService signal notification.
 The user of the class need to implement the protocol method.
 */
__deprecated
@protocol AJNAnnouncementListener <NSObject>

/**
 Called by the AnnounceHandlerAdapter when a new announcement received. This gives the listener implementation the opportunity to save a reference to announcemet parameters.
 @param version The version of the AboutService.
 @param port The port used by the AboutService
 @param busName Unique or well-known name of AllJoyn bus.
 @param objectDescs Map of ObjectDescriptions using NSMutableDictionary, describing interfaces
 @param aboutData Map of AboutData using NSMutableDictionary, describing message args
 */
- (void)announceWithVersion:(uint16_t)version port:(uint16_t)port busName:(NSString *)busName objectDescriptions:(NSMutableDictionary *)objectDescs aboutData:(NSMutableDictionary **)aboutData __deprecated;
@end