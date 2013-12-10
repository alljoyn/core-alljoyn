////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

#import "AJNSessionListenerImpl.h"

using namespace ajn;

/**
 * Constructor for the AJN session listener implementation.
 *
 * @param aBusAttachment    Objective C bus attachment wrapper object.
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
 */    
AJNSessionListenerImpl::AJNSessionListenerImpl(AJNBusAttachment *aBusAttachment, id<AJNSessionListener> aDelegate) :
    m_delegate(aDelegate), busAttachment(aBusAttachment)
{
    
}

/**
 * Virtual destructor for derivable class.
 */
AJNSessionListenerImpl::~AJNSessionListenerImpl()
{
    m_delegate = nil;
    busAttachment = nil;
}

/**
 * Called by the bus when an existing session becomes disconnected.
 *
 * @param sessionId     Id of session that was lost.
 */
void AJNSessionListenerImpl::SessionLost(SessionId sessionId)
{
    @autoreleasepool {
        if ([m_delegate respondsToSelector:@selector(sessionWasLost:)]) {
            __block id<AJNSessionListener> theDelegate = m_delegate;            
            dispatch_queue_t queue = dispatch_get_main_queue();
            dispatch_async(queue, ^{
                [theDelegate sessionWasLost:sessionId];
            });
        }
    }
}

/**
 * Called by the bus when an existing session becomes disconnected.
 *
 * @param sessionId     Id of session that was lost.
 * @param reason        The reason for the session being lost
 *
 */
void AJNSessionListenerImpl::SessionLost(SessionId sessionId, SessionLostReason reason)
{
    @autoreleasepool {
        if ([m_delegate respondsToSelector:@selector(sessionWasLost:forReason:)]) {
            __block id<AJNSessionListener> theDelegate = m_delegate;
            dispatch_queue_t queue = dispatch_get_main_queue();
            dispatch_async(queue, ^{
                [theDelegate sessionWasLost:sessionId forReason:(AJNSessionLostReason)reason];
            });
        }
    }
}

/**
 * Called by the bus when a member of a multipoint session is added.
 *
 * @param sessionId     Id of session whose member(s) changed.
 * @param uniqueName    Unique name of member who was added.
 */
void AJNSessionListenerImpl::SessionMemberAdded(SessionId sessionId, const char* uniqueName)
{
    @autoreleasepool {
        if ([m_delegate respondsToSelector:@selector(didAddMemberNamed:toSession:)]) {
            NSString *aUniqueName = [NSString stringWithCString:uniqueName encoding:NSUTF8StringEncoding];
            __block id<AJNSessionListener> theDelegate = m_delegate;                        
            dispatch_queue_t queue = dispatch_get_main_queue();
            dispatch_async(queue, ^{
                [theDelegate didAddMemberNamed:aUniqueName toSession:sessionId];
            });
        }
    }
}

/**
 * Called by the bus when a member of a multipoint session is removed.
 *
 * @param sessionId     Id of session whose member(s) changed.
 * @param uniqueName    Unique name of member who was removed.
 */
void AJNSessionListenerImpl::SessionMemberRemoved(SessionId sessionId, const char* uniqueName)
{
    @autoreleasepool {    
        if ([m_delegate respondsToSelector:@selector(didRemoveMemberNamed:fromSession:)]) {
            NSString *aUniqueName = [NSString stringWithCString:uniqueName encoding:NSUTF8StringEncoding];            
            __block id<AJNSessionListener> theDelegate = m_delegate;                        
            dispatch_queue_t queue = dispatch_get_main_queue();
            dispatch_async(queue, ^{
                [theDelegate didRemoveMemberNamed:aUniqueName fromSession:sessionId];
            });
        }
    }    
}
