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
#import "AJNObject.h"
#import "AJNBusObject.h"
#import "AJNStatus.h"
#import "AJNAboutDataListener.h"
#import "AJNMessageArgument.h"

@protocol AJNBusObject;


@interface AJNAboutObject : AJNBusObject

/**
 * create a new About class
 *
 * The about class is responsible for transmitting information about the
 * interfaces that are available for other applications to use. It also
 * provides application specific information that is contained in the
 * AboutDataListener class
 *
 * It also provides mean for applications to respond to certain requests
 * concerning the interfaces.
 *
 * By default the org.alljoyn.About interface is excluded from the list of
 * announced interfaces. Since simply receiving the announce signal tells
 * the client that the service implements the org.alljoyn.About interface.
 * There are some legacy applications that expect the org.alljoyn.About
 * interface to be part of the announcement. Changing the isAnnounced flag
 * from UNANNOUNCED, its default, to ANNOUNCED will cause the org.alljoyn.About
 * interface to be part of the announce signal. Unless your application is
 * talking with a legacy application that expects the org.alljoyn.About
 * interface to be part of the announce signal it is best to leave the
 * isAnnounced to use its default value.
 *
 * @param[in] bus the BusAttachment that will contain the about information
 * @param[in] isAnnounced will the org.alljoyn.About interface be part of the
 *                        announced interfaces.
 */
- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment withAnnounceFlag:(AJNAnnounceFlag)announceFlag;



/**
 * This is used to send the Announce signal.  It announces the list of all
 * interfaces available at given object paths as well as the announced
 * fields from the AboutData.
 *
 * This method will automatically obtain the Announced ObjectDescription from the
 * BusAttachment that was used to create the AboutObj. Only BusObjects that have
 * marked their interfaces as announced and are registered with the
 * BusAttachment will be announced.
 *
 * @see BusAttachment::RegisterBusObject
 * @see BusObject::AddInterface
 *
 * @param sessionPort the session port the interfaces can be connected with
 * @param aboutDataListener the AboutDataListener that contains the AboutData for
 *                    this announce signal.
 *
 * @return ER_OK on success
 */

- (QStatus)announceForSessionPort:(AJNSessionPort)sessionPort withAboutDataListener:(id<AJNAboutDataListener>)aboutDataListener;

/**
 * Cancel the last announce signal sent. If no signals have been sent this
 * method call will return.
 *
 * @return
 *     - ER_OK on success
 *     - annother status indicating failure.
 */

- (QStatus)unannounce;

@end
