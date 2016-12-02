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
#import "AJNAnnouncement.h"
#import "AJNTransportMask.h"

@interface ClientInformation : NSObject
@property (strong, nonatomic) AJNAnnouncement *announcement;
@property (strong, nonatomic) NSString *currLang;
@end