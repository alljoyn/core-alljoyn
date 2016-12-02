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

/** AJNAnnouncement is a storage class that holds information received from the server during announce
 */
__deprecated
@interface AJNAnnouncement : NSObject

@property uint16_t version __deprecated;
@property uint16_t port __deprecated;
@property (strong, nonatomic) NSString *busName __deprecated;
@property (strong, nonatomic) NSMutableDictionary *objectDescriptions __deprecated;
@property (strong, nonatomic) NSMutableDictionary *aboutData __deprecated;

/**
 Designated initializer
 Create AJNAnnouncement Object using the passed parameters.
 @param version The version of the AboutService.
 @param port The port used by the AboutService.
 @param busName Unique or well-known name of AllJoyn bus.
 @param objectDescs Map of ObjectDescriptions using NSMutableDictionary, describing interfaces.
 @param aboutData Map of AboutData using NSMutableDictionary.
 @return AJNAnnouncement if successful.
 */
- (id)initWithVersion:(uint16_t)version port:(uint16_t)port busName:(NSString *)busName objectDescriptions:(NSMutableDictionary *)objectDescs aboutData:(NSMutableDictionary **)aboutData __deprecated;

@end