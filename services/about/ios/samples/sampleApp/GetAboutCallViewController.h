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
