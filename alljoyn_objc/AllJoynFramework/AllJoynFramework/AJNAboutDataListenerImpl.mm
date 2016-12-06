////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
//    Source Project (AJOSP) Contributors and others.
//
//    SPDX-License-Identifier: Apache-2.0
//
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//     PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import "AJNAboutDataListenerImpl.h"
#import "AJNMessageArgument.h"
#import "AJNHandle.h"
#import "AJNAboutData.h"

using namespace ajn;

@interface AJNMessageArgument(Private)

@property (nonatomic, readonly) MsgArg *msgArg;

@end



const char * AJNAboutDataListenerImpl::AJN_ABOUT_DATA_LISTENER_DISPATCH_QUEUE_NAME = "org.alljoyn.about-data-listener.queue";

/**
 * Constructor for the AJN session port listener implementation.
 *
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.

*/

AJNAboutDataListenerImpl::AJNAboutDataListenerImpl(id<AJNAboutDataListener> aDelegate) :  m_delegate(aDelegate)
{
    
}

/**
 * Virtual destructor for derivable class.
 */
AJNAboutDataListenerImpl::~AJNAboutDataListenerImpl()
{
    m_delegate = nil;
}


/**
 * Creating the MsgArg that is returned when a user calls
 * org.alljoyn.About.GetAboutData. The returned MsgArg must contain the
 * AboutData dictionary for the Language specified.
 *
 * The MsgArg will contain the signature `a{sv}`.
 *
 * TODO add more documentation for the Key/Value pair requirements here.
 *
 * @param[out] msgArg a the dictionary containing all of the AboutData fields for
 *                    the specified language.  If language is not specified the default
 *                    language will be returned
 * @param[in] language IETF language tags specified by RFC 5646 if the string
 *                     is NULL or an empty string the MsgArg for the default
 *                     language will be returned
 *
 * @return ER_OK on successful
 */
QStatus AJNAboutDataListenerImpl::GetAboutData(MsgArg* msgArg, const char* language)
{
    QStatus status = ER_OK;
    AJNMessageArgument *ajnMsgArgContent;
    NSString *nsLang;
    
    
    if (language != nil) {
        nsLang = [NSString stringWithCString:language encoding:NSUTF8StringEncoding ];
    } else {
        nsLang = nil;
    }
        
    if ([m_delegate respondsToSelector:@selector(getAboutData:withLanguage:)]) {
        status = [m_delegate getAboutData:&ajnMsgArgContent withLanguage:nsLang];
        

    } else {
        status = ER_FAIL;
    }
    
    if(status == ER_OK){
        *msgArg = *ajnMsgArgContent.msgArg;
    }
    return status;
}

QStatus AJNAboutDataListenerImpl::GetAnnouncedAboutData(MsgArg* msgArg)
{
    QStatus status = ER_OK;
    AJNMessageArgument *ajnMsgArgContent;

    if([m_delegate respondsToSelector:@selector(getAnnouncedAboutData:)]) {
        status = [m_delegate getAnnouncedAboutData:&ajnMsgArgContent ];

    } else {
        status = ER_FAIL;
    }
    
    if(status == ER_OK){
        *msgArg = *ajnMsgArgContent.msgArg;
    }
    
    return status;
}
