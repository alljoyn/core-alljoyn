////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNSessionOptions.h"
#import "AJNMessageArgument.h"

/**
 * Protocol implemented by AllJoyn apps and called by AllJoyn to inform
 * the app of about announce related events.
 */
@protocol AJNAboutListener <NSObject>

@required
/**
 * handler for the org.alljoyn.About.Anounce sessionless signal
 *
 * The objectDescriptionArg contains an array with a signature of `a(oas)`
 * this is an array object paths with a list of interfaces found at those paths
 *
 * The aboutDataArg contains a dictionary with with AboutData fields that have
 * been announced.
 *
 * These fields are
 *  - AppId
 *  - DefaultLanguage
 *  - DeviceName
 *  - DeviceId
 *  - AppName
 *  - Manufacturer
 *  - ModelNumber
 *
 * The DeviceName is optional an may not be included in the aboutDataArg
 *
 * DeviceName, AppName, Manufacturer are localizable values. The localization
 * for these values in the aboutDataArg will always be for the language specified
 * in the DefaultLanguage field.
 *
 * @param[in] busName              well know name of the service
 * @param[in] version              version of the Announce signal from the remote About Object
 *                               About Object
 * @param[in] port                 SessionPort used by the announcer
 * @param[in] objectDescriptionArg  MsgArg the list of object paths and interfaces in the announcement
 * @param[in] aboutDataArg          MsgArg containing a dictionary of Key/Value pairs of the AboutData
 */
- (void)didReceiveAnnounceOnBus:(NSString *)busName withVersion:(uint16_t)version withSessionPort:(AJNSessionPort)port withObjectDescription:(AJNMessageArgument *)objectDescriptionArg withAboutDataArg:(AJNMessageArgument *)aboutDataArg;

@end