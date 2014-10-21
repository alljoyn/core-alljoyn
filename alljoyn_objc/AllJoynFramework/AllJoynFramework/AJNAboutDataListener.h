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
 * Protocol implemented by AllJoyn apps and called by AllJoyn to fetch
 * the about data provided by the user
 */
@protocol AJNAboutDataListener <NSObject>

@required

/**
 * Creating the dictionary that is returned when a user calls
 * org.alljoyn.About.GetAboutData. The returned MsgArg must contain the
 * AboutData dictionary for the Language specified.
 *
 * The Dictionary will contain the signature key value pairs where the key is a string 
 * value a AJNMessageArgument. The value should not be a variant since the bindings 
 * currently do not support variants. Instead providing an exact type for the message 
 * argument would work
 *
 * TODO add more documentation for the Key/Value pair requirements here.
 *
 * @param[out] aboutData the dictionary containing all of the AboutData fields for
 *                    the specified language.  If language is not specified the default
 *                    language will be returned
 * @param[in] language IETF language tags specified by RFC 5646 if the string
 *                     is NULL or an empty string the MsgArg for the default
 *                     language will be returned
 *
 * @return ER_OK on successful
 */
- (QStatus)getAboutDataForLanguage:(NSString *)language usingDictionary:(NSMutableDictionary **)aboutData;


/**
 * Return a dictionary pointer containing dictionary containing the AboutData that
 * is announced with the org.alljoyn.About.announce signal.
 * This will always be the default language and will only contain the fields
 * that are announced.
 *
 * The fields required to be part of the announced data are:
 *  - AppId
 *  - DefaultLanguage
 *  - DeviceName
 *  - DeviceId
 *  - AppName
 *  - Manufacture
 *  - ModelNumber
 *
 * If you require other fields or need the localized AboutData
 *   The org.alljoyn.About.GetAboutData method can be used.
 *
 *
 * @param[out] aboutData a dictionary with the a{sv} that contains the Announce
 *                    data.
 * @return ER_OK if successful
 */
- (QStatus)getDefaultAnnounceData:(NSMutableDictionary **)aboutData;


@end
