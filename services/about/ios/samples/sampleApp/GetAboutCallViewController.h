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
//

#import <UIKit/UIKit.h>

#import "ViewController.h"
#import "AJNAnnouncementReceiver.h"
#import "AJNAnnouncement.h"
#import "AJNConvertUtil.h"
#import "AJNAboutDataConverter.h"
#import "AJNMessageArgument.h"
#import "AJNAboutClient.h"

#import <SystemConfiguration/CaptiveNetwork.h>
#import <CoreFoundation/CFDictionary.h>

#import "AJNAboutIconService.h"
#import "AJNAboutServiceApi.h"
#import "AJNAboutService.h"
#import "AJNVersion.h"
#import "AJNProxyBusObject.h"

#import "GetAboutCallViewController.h"
#import "ClientInformation.h"

@interface GetAboutCallViewController : UIViewController

@property (weak, nonatomic) ClientInformation *clientInformation;
@property (weak, nonatomic) AJNBusAttachment *clientBusAttachment;


@end