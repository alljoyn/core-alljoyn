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

#import "AJNAboutListenerImpl.h"
#import "AJNMessageArgument.h"
#import "AJNHandle.h"

using namespace ajn;

@interface AJNMessageArgument(Private)

@property (nonatomic, readonly) MsgArg *msgArg;

@end


const char * AJNAboutListenerImpl::AJN_ABOUT_LISTENER_DISPATCH_QUEUE_NAME = "org.alljoyn.about-listener.queue";

/**
 * Constructor for the AJNAboutlistener implementation.
 *
 * @param aBusAttachment    Objective C bus attachment wrapper object.
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
 */
AJNAboutListenerImpl::AJNAboutListenerImpl(AJNBusAttachment *aBusAttachment, id<AJNAboutListener> aDelegate) : busAttachment(aBusAttachment), m_delegate(aDelegate)
{
    
}

/**
 * Virtual destructor for derivable class.
 */
AJNAboutListenerImpl::~AJNAboutListenerImpl()
{
    busAttachment = nil;
    m_delegate = nil;
}

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
 * @param[in] busName              well know name of the remote BusAttachment
 * @param[in] version              version of the Announce signal from the remote About Object
 * @param[in] port                 SessionPort used by the announcer
 * @param[in] objectDescriptionArg  MsgArg the list of object paths and interfaces in the announcement
 * @param[in] aboutDataArg          MsgArg containing a dictionary of Key/Value pairs of the AboutData
 */

void AJNAboutListenerImpl::Announced(const char* busName, uint16_t version, ajn::SessionPort port, const ajn::MsgArg& objectDescriptionArg, const ajn::MsgArg& aboutDataArg)
{
    @autoreleasepool {
        if ([m_delegate respondsToSelector:@selector(didReceiveAnnounceOnBus:withVersion:withSessionPort:withObjectDescription:withAboutDataArg:
)]) {
            NSString *nsBusName = [NSString stringWithCString:busName encoding:NSUTF8StringEncoding];
            uint16_t announceVersion = version;
            AJNSessionPort sessionPort = port;
            __block id<AJNAboutListener> theDelegate = m_delegate;
            AJNMessageArgument *objdesc = [[AJNMessageArgument alloc] init];
            *objdesc.msgArg = objectDescriptionArg;
            AJNMessageArgument *aboutData = [[AJNMessageArgument alloc] init];
            *aboutData.msgArg = aboutDataArg;
            dispatch_queue_t queue = dispatch_get_main_queue();
            dispatch_async(queue, ^{
                [theDelegate didReceiveAnnounceOnBus:nsBusName withVersion:announceVersion withSessionPort:sessionPort withObjectDescription:objdesc withAboutDataArg:aboutData];
            });
        }
    }
}

