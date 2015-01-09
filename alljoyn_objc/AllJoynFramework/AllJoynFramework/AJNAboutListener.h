////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
